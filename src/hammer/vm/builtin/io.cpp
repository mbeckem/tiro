#include "hammer/vm/builtin/modules.hpp"

#include "hammer/vm/builtin/module_builder.hpp"
#include "hammer/vm/context.hpp"
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
    TcpListener(asio::io_context& io)
        : acceptor_(io) {}

    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(const TcpListener&) = delete;

    TcpListenerState state() const { return state_; }

    void listen(asio::ip::tcp::endpoint endpoint) {
        HAMMER_CHECK(state_ == TcpListenerState::Init,
            "Cannot open this listener again.");

        try {
            acceptor_.open(endpoint.protocol());
            acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
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
        HAMMER_CHECK(
            state_ == TcpListenerState::Listening, "Not in listening state.");
        HAMMER_CHECK(!in_accept_, "Cannot accept more than once at a time.");
        acceptor_.async_accept(
            [self = self_ptr(), cb = std::move(callback)](
                std::error_code ec, asio::ip::tcp::socket peer) mutable {
                self->in_accept_ = false;
                cb(ec, std::move(peer));
            });
        in_accept_ = true;
    }

private:
    auto self_ptr() { return shared_from_this(); }

private:
    // TODO timeout support
    asio::ip::tcp::acceptor acceptor_;
    TcpListenerState state_ = TcpListenerState::Init;
    bool in_accept_ = false;
};

using TcpListenerPtr = std::shared_ptr<TcpListener>;

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

// Creates a tuple with a single member - the native object containing a pointer
// to the native listener. The tuple is accessed by the native functions
// to retrieve the native instance.
// This is a workaround be cause the vm currently lacks classes.
static Tuple listener_closure(Context& ctx, const TcpListenerPtr& listener) {
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

static void listener_open(NativeFunction::Frame& frame) {
    Root closure(frame.ctx(), frame.values());
    TcpListenerPtr listener = listener_from_closure(closure);

    const auto addr = asio::ip::make_address_v4("0.0.0.0");
    const u16 port = 12345;
    listener->listen(asio::ip::tcp::endpoint(addr, port));
}

static void listener_close(NativeFunction::Frame& frame) {
    Root closure(frame.ctx(), frame.values());
    TcpListenerPtr listener = listener_from_closure(closure);
    listener->close();
}

static void listener_state(NativeFunction::Frame& frame) {
    Root closure(frame.ctx(), frame.values());
    TcpListenerPtr listener = listener_from_closure(closure);

    Root state(frame.ctx(),
        frame.ctx().get_interned_string(to_string(listener->state())));
    frame.result(state);
}

static void listener_accept(NativeAsyncFunction::Frame frame) {
    Root closure(frame.ctx(), frame.values());
    TcpListenerPtr listener = listener_from_closure(closure);

    listener->accept([frame = std::move(frame)](std::error_code ec,
                         asio::ip::tcp::socket peer) mutable {
        fmt::memory_buffer buf;
        if (ec) {
            fmt::format_to(
                buf, "Error accepting a new connection: {}", ec.message());
        } else {
            auto endpoint = peer.remote_endpoint();
            fmt::format_to(buf, "Accepted a connection from: {}:{}",
                endpoint.address().to_string(), endpoint.port());
        }

        std::error_code ignored;
        peer.close(ignored);

        frame.result(String::make(frame.ctx(), to_string(buf)));
        frame.resume();
    });
}

static void listener_create(NativeFunction::Frame& frame) {
    Context& ctx = frame.ctx();

    TcpListenerPtr native_listener = std::make_shared<TcpListener>(
        ctx.io_context());
    Root<Tuple> closure(ctx, listener_closure(ctx, native_listener));

    ObjectBuilder builder(ctx, closure);
    builder.add_func("open", 0, listener_open)
        .add_func("close", 0, listener_close)
        .add_func("state", 0, listener_state)
        .add_async_func("accept", 0, listener_accept);
    frame.result(builder.build());
}

Module create_io_module(Context& ctx) {
    ModuleBuilder builder(ctx, "io");
    builder.add_function("new_listener", 0, {}, listener_create);
    return builder.build();
}

} // namespace hammer::vm
