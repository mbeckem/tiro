#include "hammer/vm/builtin/modules.hpp"

#include "hammer/vm/builtin/module_builder.hpp"
#include "hammer/vm/context.hpp"
#include "hammer/vm/math.hpp"
#include "hammer/vm/objects/buffers.hpp"
#include "hammer/vm/objects/classes.hpp"
#include "hammer/vm/objects/functions.hpp"
#include "hammer/vm/objects/native_objects.hpp"
#include "hammer/vm/objects/strings.hpp"

#include <asio/ts/internet.hpp>
#include <asio/ts/net.hpp>
#include <asio/ts/socket.hpp>

#include <memory>
#include <new>

namespace hammer::vm {

using asio::ip::tcp;

namespace {

/* TODO: Handle exceptions thrown by asio */
/* TODO: Rather migrate to a c based io library like: would make abi compat
         easier for shared libraries */

enum class TcpListenerState { Init, Listening, Closed };

std::string_view to_string(TcpListenerState state) {
    switch (state) {
    case TcpListenerState::Init:
        return "Init";
    case TcpListenerState::Listening:
        return "Listening";
    case TcpListenerState::Closed:
        return "Closed";
    }

    HAMMER_UNREACHABLE("Invalid listener state.");
}

class TcpListener : public std::enable_shared_from_this<TcpListener> {
public:
    explicit TcpListener(asio::io_context& io)
        : acceptor_(io) {}

    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(const TcpListener&) = delete;

    TcpListenerState state() const { return state_; }

    bool reuse_address() const { return reuse_address_; }

    void reuse_address(bool reuse) {
        HAMMER_CHECK(state_ == TcpListenerState::Init,
            "Cannot change this property after initializiation phase.");
        reuse_address_ = reuse;
    }

    void listen(tcp::endpoint endpoint) {
        HAMMER_CHECK(state_ == TcpListenerState::Init,
            "Cannot open this listener again.");

        try {
            acceptor_.open(endpoint.protocol());

            if (reuse_address_) {
                acceptor_.set_option(tcp::acceptor::reuse_address(true));
            }

            acceptor_.bind(endpoint);
            acceptor_.listen();
            state_ = TcpListenerState::Listening;
        } catch (const std::exception& err) {
            close();
            HAMMER_ERROR("Failed to start listening: {}", err.what());
        }
    }

    void close() noexcept {
        std::error_code ignored;
        acceptor_.close(ignored);
        state_ = TcpListenerState::Closed;
    }

    // The callback will be invoked with error code and peer.
    template<typename Callback>
    void accept(Callback&& callback) {
        HAMMER_CHECK(!in_accept_, "Cannot accept more than once at a time.");
        acceptor_.async_accept(
            [self = self_ptr(), cb = std::move(callback)](
                std::error_code ec, tcp::socket peer) mutable {
                self->in_accept_ = false;
                cb(ec, std::move(peer));
            });
        in_accept_ = true;
    }

private:
    auto self_ptr() { return shared_from_this(); }

private:
    // TODO timeout support
    tcp::acceptor acceptor_;
    TcpListenerState state_ = TcpListenerState::Init;
    bool reuse_address_ = false;
    bool in_accept_ = false;
};

using TcpListenerPtr = std::shared_ptr<TcpListener>;

class TcpSocket : public std::enable_shared_from_this<TcpSocket> {
public:
    explicit TcpSocket(tcp::socket socket)
        : socket_(std::move(socket)) {}

    bool is_open() const { return socket_.is_open(); }
    tcp::endpoint remote_endpoint() const { return socket_.remote_endpoint(); }
    tcp::endpoint local_endpoint() const { return socket_.local_endpoint(); }

    void enable_no_delay(bool enabled) {
        socket_.set_option(tcp::no_delay(enabled));
    }

    void close() {
        std::error_code ignored;
        socket_.close(ignored);
    }

    // Storage must remain valid for as long as the read call is pending.
    template<typename Callback>
    void read(Span<byte> data, Callback&& callback) {
        HAMMER_CHECK(!in_read_, "Cannot read more than once at a time.");

        auto buffer = asio::buffer(data.data(), data.size());
        socket_.async_read_some(
            buffer, [self = self_ptr(), cb = std::move(callback)](
                        std::error_code ec, size_t n) mutable {
                self->in_read_ = false;
                cb(ec, n);
            });
    }

    // Storage must remain valid for as long as the read call is pending.
    template<typename Callback>
    void write(Span<const byte> data, Callback&& callback) {
        HAMMER_CHECK(!in_write_, "Cannot write more than once at a time.");

        auto buffer = asio::const_buffer(data.data(), data.size());
        socket_.async_write_some(
            buffer, [self = self_ptr(), cb = std::move(callback)](
                        std::error_code ec, size_t n) mutable {
                self->in_write_ = false;
                cb(ec, n);
            });
    }

private:
    auto self_ptr() { return shared_from_this(); }

private:
    // TODO Timeout support for read / write
    tcp::socket socket_;
    bool in_read_ = false;
    bool in_write_ = false;
};

using TcpSocketPtr = std::shared_ptr<TcpSocket>;

class ObjectBuilder {
public:
    ObjectBuilder(Context& ctx, Handle<Tuple> closure)
        : ctx_(ctx)
        , closure_(closure)
        , obj_(ctx_, DynamicObject::make(ctx_)) {}

    ObjectBuilder& add_func(std::string_view name, u32 argc,
        NativeFunction::FunctionType func_ptr) {
        Root name_obj(ctx_, ctx_.get_interned_string(name));
        Root func_obj(ctx_,
            NativeFunction::make(ctx_, name_obj, closure_, argc, func_ptr));
        return add_member(name, func_obj.handle());
    }

    ObjectBuilder& add_async_func(std::string_view name, u32 argc,
        NativeAsyncFunction::FunctionType func_ptr) {
        Root name_obj(ctx_, ctx_.get_interned_string(name));
        Root func_obj(ctx_, NativeAsyncFunction::make(
                                ctx_, name_obj, closure_, argc, func_ptr));
        return add_member(name, func_obj.handle());
    }

    ObjectBuilder& add_member(std::string_view name, Handle<Value> member) {
        Root symbol(ctx_, ctx_.get_symbol(name));
        obj_->set(ctx_, symbol, member);
        return *this;
    }

    DynamicObject build() { return obj_; }

private:
    Context& ctx_;
    Handle<Tuple> closure_;
    Root<DynamicObject> obj_;
};

} // namespace

static void listener_create(NativeFunction::Frame& frame);
static void listener_state(NativeFunction::Frame& frame);
static void listener_reuse_address(NativeFunction::Frame& frame);
static void listener_open(NativeFunction::Frame& frame);
static void listener_close(NativeFunction::Frame& frame);
static void listener_accept(NativeAsyncFunction::Frame frame);

static Tuple
make_listener_closure(Context& ctx, const TcpListenerPtr& listener);
static TcpListenerPtr listener_from_closure(Handle<Tuple> closure);

static void socket_is_open(NativeFunction::Frame& frame);
static void socket_close(NativeFunction::Frame& frame);
static void socket_enable_no_delay(NativeFunction::Frame& frame);
static void socket_remote_endpoint(NativeFunction::Frame& frame);
static void socket_local_endpoint(NativeFunction::Frame& frame);
static void socket_read(NativeAsyncFunction::Frame frame);
static void socket_write(NativeAsyncFunction::Frame frame);

static Tuple make_socket_closure(Context& ctx, const TcpSocketPtr& socket);
static TcpSocketPtr socket_from_closure(Handle<Tuple> closure);

static std::string format_endpoint(const tcp::endpoint& ep);

static void listener_create(NativeFunction::Frame& frame) {
    Context& ctx = frame.ctx();

    TcpListenerPtr native_listener = std::make_shared<TcpListener>(
        ctx.io_context());
    Root closure(ctx, make_listener_closure(ctx, native_listener));

    ObjectBuilder builder(ctx, closure);
    builder.add_func("open", 2, listener_open)
        .add_func("close", 0, listener_close)
        .add_func("reuse_address", 1, listener_reuse_address)
        .add_func("state", 0, listener_state)
        .add_async_func("accept", 0, listener_accept);
    frame.result(builder.build());
}

static void listener_state(NativeFunction::Frame& frame) {
    Root closure(frame.ctx(), frame.values());
    TcpListenerPtr listener = listener_from_closure(closure);

    Root state(frame.ctx(),
        frame.ctx().get_interned_string(to_string(listener->state())));
    frame.result(state);
}

static void listener_reuse_address(NativeFunction::Frame& frame) {
    Root closure(frame.ctx(), frame.values());
    TcpListenerPtr listener = listener_from_closure(closure);
    listener->reuse_address(frame.ctx().is_truthy(frame.arg(0)));
}

static void listener_open(NativeFunction::Frame& frame) {
    Root closure(frame.ctx(), frame.values());
    TcpListenerPtr listener = listener_from_closure(closure);

    auto addr_str = frame.arg(0);
    auto port_int = frame.arg(1);

    asio::ip::address addr;
    u16 port = 0;

    if (addr_str->is<String>()) {
        std::string_view str = addr_str.cast<String>()->view();

        std::error_code ec;
        addr = asio::ip::make_address(str, ec);
        if (ec) {
            HAMMER_ERROR(
                "Failed to parse ip address from {}: {}", str, ec.message());
        }
    } else {
        HAMMER_ERROR("`ip` must be a valid ip address string.");
    }

    if (auto num = try_extract_integer(port_int)) {
        if (*num < 0 || *num > (1 << 16))
            HAMMER_ERROR("`port` out of range: {}", *num);
        port = *num;
    } else {
        HAMMER_ERROR("`port` must be a valid integer.");
    }

    listener->listen(tcp::endpoint(addr, port));
}

static void listener_close(NativeFunction::Frame& frame) {
    Root closure(frame.ctx(), frame.values());
    TcpListenerPtr listener = listener_from_closure(closure);
    listener->close();
}

static void listener_accept(NativeAsyncFunction::Frame frame) {
    Root closure(frame.ctx(), frame.values());
    TcpListenerPtr listener = listener_from_closure(closure);

    listener->accept([frame = std::move(frame)](
                         std::error_code ec, tcp::socket peer) mutable {
        if (ec) {
            // TODO Exceptions
            HAMMER_ERROR("Failed to accept new connection: {}", ec.message());
        }

        Context& ctx = frame.ctx();
        TcpSocketPtr native_socket = std::make_shared<TcpSocket>(
            std::move(peer));
        Root new_closure(ctx, make_socket_closure(ctx, native_socket));

        ObjectBuilder builder(ctx, new_closure);
        builder.add_func("is_open", 0, socket_is_open)
            .add_func("close", 0, socket_close)
            .add_func("enable_no_delay", 1, socket_enable_no_delay)
            .add_func("remote_endpoint", 0, socket_remote_endpoint)
            .add_func("local_endpoint", 0, socket_local_endpoint)
            .add_async_func("write", 3, socket_write)
            .add_async_func("read", 3, socket_read);

        frame.result(builder.build());
        frame.resume();
    });
}

// Creates a tuple with a single member - the native object containing a pointer
// to the native listener. The tuple is accessed by the native functions
// to retrieve the native instance.
// This is a workaround be cause the vm currently lacks classes.
static Tuple
make_listener_closure(Context& ctx, const TcpListenerPtr& listener) {
    Root<Tuple> closure(ctx, Tuple::make(ctx, 1));
    Root<NativeObject> object(
        ctx, NativeObject::make(ctx, sizeof(TcpListenerPtr)));

    new (object->data()) TcpListenerPtr(listener);
    object->set_finalizer([](void* data, [[maybe_unused]] size_t size) {
        HAMMER_ASSERT(
            size == sizeof(TcpListenerPtr), "Invalid size of native object.");
        static_cast<TcpListenerPtr*>(data)->~TcpListenerPtr();
    });

    closure->set(0, object);
    return closure;
}

// Returns the listener stored at index 0 in the closure tuple.
static TcpListenerPtr listener_from_closure(Handle<Tuple> closure) {
    void* data = closure->get(0).as<NativeObject>().data();
    return *static_cast<TcpListenerPtr*>(data);
}

static void socket_is_open(NativeFunction::Frame& frame) {
    Root closure(frame.ctx(), frame.values());
    TcpSocketPtr socket = socket_from_closure(closure);
    frame.result(frame.ctx().get_boolean(socket->is_open()));
}

static void socket_close(NativeFunction::Frame& frame) {
    Root closure(frame.ctx(), frame.values());
    TcpSocketPtr socket = socket_from_closure(closure);
    socket->close();
}

static void socket_enable_no_delay(NativeFunction::Frame& frame) {
    Root closure(frame.ctx(), frame.values());
    TcpSocketPtr socket = socket_from_closure(closure);
    socket->enable_no_delay(frame.ctx().is_truthy(frame.arg(0)));
}

static void socket_remote_endpoint(NativeFunction::Frame& frame) {
    Root closure(frame.ctx(), frame.values());
    TcpSocketPtr socket = socket_from_closure(closure);
    std::string endpoint = format_endpoint(socket->remote_endpoint());
    frame.result(String::make(frame.ctx(), endpoint));
}

static void socket_local_endpoint(NativeFunction::Frame& frame) {
    Root closure(frame.ctx(), frame.values());
    TcpSocketPtr socket = socket_from_closure(closure);
    std::string endpoint = format_endpoint(socket->local_endpoint());
    frame.result(String::make(frame.ctx(), endpoint));
}

// Returns true iff [start, start + n) can fit into `size`.
static bool range_check(size_t size, size_t start, size_t n) {
    if (start > size)
        return false;
    return n <= size - start; // start + n <= size
}

static Span<byte> get_pinned_span(Context& ctx, Handle<Value> buffer_param,
    Handle<Value> start_param, Handle<Value> count_param) {
    HAMMER_CHECK(
        buffer_param->is<Buffer>(), "`buffer` must be a valid byte buffer.");
    HAMMER_CHECK(ctx.heap().is_pinned(buffer_param),
        "`buffer` must be pinned in memory.");

    auto span = buffer_param.cast<Buffer>()->values();
    auto start = try_extract_size(start_param);
    auto count = try_extract_size(count_param);
    HAMMER_CHECK(start, "`start` must be a valid integer.");
    HAMMER_CHECK(count, "`count` must be a valid integer.");
    HAMMER_CHECK(range_check(span.size(), *start, *count),
        "Invalid range indices for the size of `buffer`.");

    return span.subspan(*start, *count);
}

static void socket_read(NativeAsyncFunction::Frame frame) {
    auto span = get_pinned_span(
        frame.ctx(), frame.arg(0), frame.arg(1), frame.arg(2));

    HAMMER_CHECK(span.size() > 0, "Cannot execute zero sized reads.");

    Root closure(frame.ctx(), frame.values());
    TcpSocketPtr socket = socket_from_closure(closure);
    socket->read(span, [frame = std::move(frame)](
                           std::error_code ec, size_t n) mutable {
        if (ec) {
            // TODO Exceptions
            HAMMER_ERROR("Failed to read from tcp socket: {}", ec.message());
        }

        frame.result(frame.ctx().get_integer(static_cast<i64>(n)));
        frame.resume();
    });
}

static void socket_write(NativeAsyncFunction::Frame frame) {
    auto span = get_pinned_span(
        frame.ctx(), frame.arg(0), frame.arg(1), frame.arg(2));

    Root closure(frame.ctx(), frame.values());
    TcpSocketPtr socket = socket_from_closure(closure);
    socket->write(
        span, [frame = std::move(frame)](std::error_code ec, size_t n) mutable {
            if (ec) {
                // TODO Exceptions
                HAMMER_ERROR("Failed to write to tcp socket: {}", ec.message());
            }

            frame.result(frame.ctx().get_integer(static_cast<i64>(n)));
            frame.resume();
        });
}

static Tuple make_socket_closure(Context& ctx, const TcpSocketPtr& socket) {
    Root<Tuple> closure(ctx, Tuple::make(ctx, 1));
    Root<NativeObject> object(
        ctx, NativeObject::make(ctx, sizeof(TcpSocketPtr)));

    new (object->data()) TcpSocketPtr(socket);
    object->set_finalizer([](void* data, [[maybe_unused]] size_t size) {
        HAMMER_ASSERT(
            size == sizeof(TcpSocketPtr), "Invalid size of native object.");
        static_cast<TcpSocketPtr*>(data)->~TcpSocketPtr();
    });

    closure->set(0, object);
    return closure;
}

static TcpSocketPtr socket_from_closure(Handle<Tuple> closure) {
    void* data = closure->get(0).as<NativeObject>().data();
    return *static_cast<TcpSocketPtr*>(data);
}

std::string format_endpoint(const tcp::endpoint& ep) {
    return fmt::format("{}:{}", ep.address().to_string(), ep.port());
}

Module create_io_module(Context& ctx) {
    ModuleBuilder builder(ctx, "std.io");
    builder.add_function("new_listener", 0, {}, listener_create);
    return builder.build();
}

} // namespace hammer::vm
