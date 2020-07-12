#include "vm/modules/modules.hpp"

#include "common/ref_counted.hpp"
#include "vm/context.hpp"
#include "vm/math.hpp"
#include "vm/modules/module_builder.hpp"
#include "vm/objects/buffer.hpp"
#include "vm/objects/class.hpp"
#include "vm/objects/function.hpp"
#include "vm/objects/native.hpp"
#include "vm/objects/string.hpp"

#include <asio/ts/internet.hpp>
#include <asio/ts/net.hpp>
#include <asio/ts/socket.hpp>

#include <memory>
#include <new>

namespace tiro::vm {

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

    TIRO_UNREACHABLE("Invalid listener state.");
}

class TcpListener : public RefCounted {
public:
    explicit TcpListener(asio::io_context& io)
        : acceptor_(io) {}

    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(const TcpListener&) = delete;

    TcpListenerState state() const { return state_; }

    bool reuse_address() const { return reuse_address_; }

    void reuse_address(bool reuse) {
        TIRO_CHECK(state_ == TcpListenerState::Init,
            "Cannot change this property after initializiation phase.");
        reuse_address_ = reuse;
    }

    void listen(const tcp::endpoint& endpoint) {
        TIRO_CHECK(state_ == TcpListenerState::Init, "Cannot open this listener again.");

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
            TIRO_ERROR("Failed to start listening: {}", err.what());
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
        TIRO_CHECK(!in_accept_, "Cannot accept more than once at a time.");
        acceptor_.async_accept([self = Ref(this), cb = std::forward<Callback>(callback)](
                                   std::error_code ec, tcp::socket peer) mutable {
            self->in_accept_ = false;
            cb(ec, std::move(peer));
        });
        in_accept_ = true;
    }

private:
    // TODO timeout support
    tcp::acceptor acceptor_;
    TcpListenerState state_ = TcpListenerState::Init;
    bool reuse_address_ = false;
    bool in_accept_ = false;
};

class TcpSocket : public RefCounted {
public:
    explicit TcpSocket(tcp::socket socket)
        : socket_(std::move(socket)) {}

    bool is_open() const { return socket_.is_open(); }
    tcp::endpoint remote_endpoint() const { return socket_.remote_endpoint(); }
    tcp::endpoint local_endpoint() const { return socket_.local_endpoint(); }

    void enable_no_delay(bool enabled) { socket_.set_option(tcp::no_delay(enabled)); }

    void close() {
        std::error_code ignored;
        socket_.close(ignored);
    }

    // Storage must remain valid for as long as the read call is pending.
    template<typename Callback>
    void read(Span<byte> data, Callback&& callback) {
        TIRO_CHECK(!in_read_, "Cannot read more than once at a time.");

        auto buffer = asio::buffer(data.data(), data.size());
        socket_.async_read_some(buffer, [self = Ref(this), cb = std::forward<Callback>(callback)](
                                            std::error_code ec, size_t n) mutable {
            self->in_read_ = false;
            cb(ec, n);
        });
    }

    // Storage must remain valid for as long as the read call is pending.
    template<typename Callback>
    void write(Span<const byte> data, Callback&& callback) {
        TIRO_CHECK(!in_write_, "Cannot write more than once at a time.");

        auto buffer = asio::const_buffer(data.data(), data.size());
        socket_.async_write_some(buffer, [self = Ref(this), cb = std::forward<Callback>(callback)](
                                             std::error_code ec, size_t n) mutable {
            self->in_write_ = false;
            cb(ec, n);
        });
    }

private:
    // TODO Timeout support for read / write
    tcp::socket socket_;
    bool in_read_ = false;
    bool in_write_ = false;
};

class ObjectBuilder {
public:
    ObjectBuilder(Context& ctx, Handle<Tuple> closure)
        : ctx_(ctx)
        , sc_(ctx)
        , closure_(closure)
        , obj_(sc_.local(DynamicObject::make(ctx_))) {}

    ObjectBuilder& add_func(std::string_view name, u32 argc, NativeFunctionPtr func_ptr) {
        Scope sc(ctx_);
        Local name_obj = sc.local(ctx_.get_interned_string(name));
        Local func_obj = sc.local(NativeFunction::make(ctx_, name_obj, closure_, argc, func_ptr));
        return add_member(name, func_obj);
    }

    ObjectBuilder&
    add_async_func(std::string_view name, u32 argc, NativeAsyncFunctionPtr func_ptr) {
        Scope sc(ctx_);
        Local name_obj = sc.local(ctx_.get_interned_string(name));
        Local func_obj = sc.local(NativeFunction::make(ctx_, name_obj, closure_, argc, func_ptr));
        return add_member(name, func_obj);
    }

    ObjectBuilder& add_member(std::string_view name, Handle<Value> member) {
        Scope sc(ctx_);
        Local symbol = sc.local(ctx_.get_symbol(name));
        obj_->set(ctx_, symbol, member);
        return *this;
    }

    DynamicObject build() { return *obj_; }

private:
    Context& ctx_;
    Scope sc_;
    Handle<Tuple> closure_;
    Local<DynamicObject> obj_;
};

} // namespace

static void listener_create(NativeFunctionFrame& frame);
static void listener_state(NativeFunctionFrame& frame);
static void listener_reuse_address(NativeFunctionFrame& frame);
static void listener_open(NativeFunctionFrame& frame);
static void listener_close(NativeFunctionFrame& frame);
static void listener_accept(NativeAsyncFunctionFrame frame);

static Tuple make_listener_closure(Context& ctx, TcpListener* listener);
static Ref<TcpListener> listener_from_closure(Nullable<Tuple> closure);

static void socket_is_open(NativeFunctionFrame& frame);
static void socket_close(NativeFunctionFrame& frame);
static void socket_enable_no_delay(NativeFunctionFrame& frame);
static void socket_remote_endpoint(NativeFunctionFrame& frame);
static void socket_local_endpoint(NativeFunctionFrame& frame);
static void socket_read(NativeAsyncFunctionFrame frame);
static void socket_write(NativeAsyncFunctionFrame frame);

static Tuple make_socket_closure(Context& ctx, TcpSocket* socket);
static Ref<TcpSocket> socket_from_closure(Nullable<Tuple> closure);

static std::string format_endpoint(const tcp::endpoint& ep);

static void listener_create(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();

    Ref<TcpListener> native_listener = make_ref<TcpListener>(ctx.io_context());

    Scope sc(ctx);
    Local closure = sc.local(make_listener_closure(ctx, native_listener));

    ObjectBuilder builder(ctx, closure);
    builder.add_func("open", 2, listener_open)
        .add_func("close", 0, listener_close)
        .add_func("reuse_address", 1, listener_reuse_address)
        .add_func("state", 0, listener_state)
        .add_async_func("accept", 0, listener_accept);
    frame.result(builder.build());
}

static void listener_state(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    Scope sc(ctx);

    Ref<TcpListener> listener = listener_from_closure(frame.values());
    std::string_view tcp_state = to_string(listener->state());
    Local state = sc.local(ctx.get_interned_string(tcp_state));
    frame.result(*state);
}

static void listener_reuse_address(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    Ref<TcpListener> listener = listener_from_closure(frame.values());
    listener->reuse_address(ctx.is_truthy(frame.arg(0)));
}

static void listener_open(NativeFunctionFrame& frame) {
    Ref<TcpListener> listener = listener_from_closure(frame.values());
    auto addr_arg = frame.arg(0);
    auto port_arg = frame.arg(1);

    asio::ip::address addr;
    u16 port = 0;

    if (auto maybe_addr_str = addr_arg.try_cast<String>()) {
        auto addr_str = maybe_addr_str.handle();

        std::error_code ec;
        addr = asio::ip::make_address(addr_str->view(), ec);
        if (ec) {
            TIRO_ERROR("Failed to parse ip address from '{}': {}.", addr_str->view(), ec.message());
        }
    } else {
        frame.result(String::make(frame.ctx(), "Expected a valid ip address string."));
        return;
    }

    if (auto num = try_extract_integer(*port_arg)) {
        if (*num < 0 || *num > (1 << 16)) {
            TIRO_ERROR("Port '{}' is out of range.", *num);
        }

        port = *num;
    } else {
        TIRO_ERROR("Port must be an integer.");
    }

    listener->listen(tcp::endpoint(addr, port));
}

static void listener_close(NativeFunctionFrame& frame) {
    Ref<TcpListener> listener = listener_from_closure(frame.values());
    listener->close();
}

static void listener_accept(NativeAsyncFunctionFrame frame) {
    Ref<TcpListener> listener = listener_from_closure(frame.values());

    listener->accept([frame = std::move(frame)](std::error_code ec, tcp::socket peer) mutable {
        Context& ctx = frame.ctx();
        Scope sc(ctx);
        Local result = sc.local(Tuple::make(frame.ctx(), 2));

        if (ec) {
            std::string error = fmt::format("Failed to accept a new connection: {}.", ec.message());

            result->set(1, String::make(frame.ctx(), error));
            return frame.result(*result);
        }

        Ref<TcpSocket> native_socket = make_ref<TcpSocket>(std::move(peer));
        Local new_closure = sc.local(make_socket_closure(ctx, native_socket));

        ObjectBuilder builder(ctx, new_closure);
        builder.add_func("is_open", 0, socket_is_open)
            .add_func("close", 0, socket_close)
            .add_func("enable_no_delay", 1, socket_enable_no_delay)
            .add_func("remote_endpoint", 0, socket_remote_endpoint)
            .add_func("local_endpoint", 0, socket_local_endpoint)
            .add_async_func("write", 3, socket_write)
            .add_async_func("read", 3, socket_read);

        result->set(0, builder.build());
        return frame.result(*result);
    });
}

// Creates a tuple with a single member - the native object containing a pointer
// to the native listener. The tuple is accessed by the native functions
// to retrieve the native instance.
// This is a workaround be cause the vm currently lacks classes.
static Tuple make_listener_closure(Context& ctx, TcpListener* listener) {
    Scope sc(ctx);
    Local closure = sc.local(Tuple::make(ctx, 1));
    Local object = sc.local(NativeObject::make(ctx, sizeof(Ref<TcpListener>)));

    new (object->data()) Ref<TcpListener>(listener);
    object->finalizer([](void* data, [[maybe_unused]] size_t size) {
        TIRO_DEBUG_ASSERT(size == sizeof(Ref<TcpListener>), "Invalid size of native object.");
        static_cast<Ref<TcpListener>*>(data)->~Ref<TcpListener>();
    });

    closure->set(0, *object);
    return *closure;
}

// Returns the listener stored at index 0 in the closure tuple.
static Ref<TcpListener> listener_from_closure(Nullable<Tuple> closure) {
    TIRO_CHECK(closure && closure.value().size() > 0, "Invalid listener closure.");
    void* data = closure.value().get(0).must_cast<NativeObject>().data();
    return *static_cast<Ref<TcpListener>*>(data);
}

static void socket_is_open(NativeFunctionFrame& frame) {
    Ref<TcpSocket> socket = socket_from_closure(frame.values());
    frame.result(frame.ctx().get_boolean(socket->is_open()));
}

static void socket_close(NativeFunctionFrame& frame) {
    Ref<TcpSocket> socket = socket_from_closure(frame.values());
    socket->close();
}

static void socket_enable_no_delay(NativeFunctionFrame& frame) {
    Ref<TcpSocket> socket = socket_from_closure(frame.values());
    socket->enable_no_delay(frame.ctx().is_truthy(frame.arg(0)));
}

static void socket_remote_endpoint(NativeFunctionFrame& frame) {
    Ref<TcpSocket> socket = socket_from_closure(frame.values());
    std::string endpoint = format_endpoint(socket->remote_endpoint());
    frame.result(String::make(frame.ctx(), endpoint));
}

static void socket_local_endpoint(NativeFunctionFrame& frame) {
    Ref<TcpSocket> socket = socket_from_closure(frame.values());
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
    TIRO_CHECK(buffer_param->is<Buffer>(), "`buffer` must be a valid byte buffer.");
    TIRO_CHECK(ctx.heap().is_pinned(*buffer_param), "`buffer` must be pinned in memory.");

    auto span = buffer_param.must_cast<Buffer>()->values();
    auto start = try_extract_size(*start_param);
    auto count = try_extract_size(*count_param);
    TIRO_CHECK(start, "`start` must be a valid integer.");
    TIRO_CHECK(count, "`count` must be a valid integer.");
    TIRO_CHECK(range_check(span.size(), *start, *count),
        "Invalid range indices for the size of `buffer`.");

    return span.subspan(*start, *count);
}

static void socket_read(NativeAsyncFunctionFrame frame) {
    auto span = get_pinned_span(frame.ctx(), frame.arg(0), frame.arg(1), frame.arg(2));
    TIRO_CHECK(!span.empty(), "Cannot execute zero sized reads.");
    Ref<TcpSocket> socket = socket_from_closure(frame.values());

    socket->read(span, [frame = std::move(frame)](std::error_code ec, size_t n) mutable {
        Context& ctx = frame.ctx();
        Scope sc(ctx);
        Local result = sc.local(Tuple::make(frame.ctx(), 2));
        if (ec) {
            // Closure value 1 is the symbol EOF, see construction of the socket object.
            // This is just a temporary solution until we make a real IO module.
            if (ec == asio::error::eof) {
                Local closure = sc.local(frame.values());
                result->set(1, closure.must_cast<Tuple>()->get(1));
            } else {
                std::string error = fmt::format(
                    "Failed to read from tcp socket: {}.", ec.message());
                result->set(1, String::make(frame.ctx(), error));
            }

            return frame.result(*result);
        }

        result->set(0, frame.ctx().get_integer(static_cast<i64>(n)));
        return frame.result(*result);
    });
}

static void socket_write(NativeAsyncFunctionFrame frame) {
    auto span = get_pinned_span(frame.ctx(), frame.arg(0), frame.arg(1), frame.arg(2));
    Ref<TcpSocket> socket = socket_from_closure(frame.values());

    socket->write(span, [frame = std::move(frame)](std::error_code ec, size_t n) mutable {
        Context& ctx = frame.ctx();
        Scope sc(ctx);
        Local result = sc.local(Tuple::make(frame.ctx(), 2));
        if (ec) {
            std::string error = fmt::format("Failed to write to tcp socket: {}.", ec.message());
            result->set(1, String::make(frame.ctx(), error));
            return frame.result(*result);
        }

        result->set(0, frame.ctx().get_integer(static_cast<i64>(n)));
        return frame.result(*result);
    });
}

static Tuple make_socket_closure(Context& ctx, TcpSocket* socket) {
    Scope sc(ctx);
    Local closure = sc.local(Tuple::make(ctx, 2));
    Local object = sc.local(NativeObject::make(ctx, sizeof(Ref<TcpSocket>)));

    new (object->data()) Ref<TcpSocket>(socket);
    object->finalizer([](void* data, [[maybe_unused]] size_t size) {
        TIRO_DEBUG_ASSERT(size == sizeof(Ref<TcpSocket>), "Invalid size of native object.");
        static_cast<Ref<TcpSocket>*>(data)->~Ref<TcpSocket>();
    });

    closure->set(0, *object);
    closure->set(1, ctx.get_symbol("eof"));
    return *closure;
}

static Ref<TcpSocket> socket_from_closure(Nullable<Tuple> closure) {
    TIRO_CHECK(closure && closure.value().size() > 0, "Invalid socket closure.");
    void* data = closure.value().get(0).must_cast<NativeObject>().data();
    return *static_cast<Ref<TcpSocket>*>(data);
}

std::string format_endpoint(const tcp::endpoint& ep) {
    return fmt::format("{}:{}", ep.address().to_string(), ep.port());
}

Module create_io_module(Context& ctx) {
    ModuleBuilder builder(ctx, "std.io");
    builder.add_function("new_listener", 0, {}, listener_create);
    return builder.build();
}

} // namespace tiro::vm
