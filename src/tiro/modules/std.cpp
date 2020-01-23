#include "tiro/modules/modules.hpp"

#include "tiro/core/ref_counted.hpp"
#include "tiro/modules/module_builder.hpp"
#include "tiro/objects/buffers.hpp"
#include "tiro/objects/classes.hpp"
#include "tiro/objects/strings.hpp"

#include "tiro/vm/context.ipp"
#include "tiro/vm/math.hpp"

#include <asio/ts/timer.hpp>

#include <cstdio>
#include <limits>

namespace tiro::vm {

namespace {

class Timer : public RefCounted {
public:
    explicit Timer(asio::io_context& io)
        : timer_(io) {}

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    void timeout_in(i64 millis) {
        millis = std::max(millis, i64(0));
        timer_.expires_after(std::chrono::milliseconds(millis));
    }

    template<typename Callback>
    void wait(Callback&& callback) {
        TIRO_CHECK(!in_wait_, "Cannot wait more than once at a time.");
        timer_.async_wait(
            [self = Ref(this), cb = std::forward<Callback>(callback)](
                std::error_code ec) mutable {
                self->in_wait_ = false;
                cb(ec);
            });
    }

private:
    asio::steady_timer timer_;
    bool in_wait_ = false;
};

} // namespace

static void print(NativeFunction::Frame& frame) {
    const size_t args = frame.arg_count();

    Context& ctx = frame.ctx();
    Root<StringBuilder> builder(ctx, StringBuilder::make(ctx));
    for (size_t i = 0; i < args; ++i) {
        if (i != 0) {
            builder->append(ctx, " ");
        }
        to_string(ctx, builder.handle(), frame.arg(i));
    }
    builder->append(ctx, "\n");

    // TODO stdout from ctx
    std::string_view message = builder->view();
    std::fwrite(message.data(), 1, message.size(), stdout);
    std::fflush(stdout);
}

static void new_string_builder(NativeFunction::Frame& frame) {
    Context& ctx = frame.ctx();
    frame.result(StringBuilder::make(ctx));
}

static void new_object(NativeFunction::Frame& frame) {
    Context& ctx = frame.ctx();
    frame.result(DynamicObject::make(ctx));
}

static void new_buffer(NativeFunction::Frame& frame) {
    Context& ctx = frame.ctx();

    size_t size = 0;
    if (auto arg = try_extract_size(frame.arg(0))) {
        size = *arg;
    } else {
        TIRO_ERROR("Invalid size argument for buffer creation.");
    }

    frame.result(Buffer::make(ctx, size, 0));
}

static void launch(NativeFunction::Frame& frame) {
    Context& ctx = frame.ctx();
    Handle func = frame.arg(0);
    frame.result(ctx.make_coroutine(func));
}

static void loop_timestamp(NativeFunction::Frame& frame) {
    Context& ctx = frame.ctx();
    frame.result(ctx.get_integer(ctx.loop_timestamp()));
}

static void sleep(NativeAsyncFunction::Frame frame) {
    Context& ctx = frame.ctx();

    i64 millis = 0;
    if (auto extracted = try_convert_integer(frame.arg(0))) {
        millis = *extracted;
    } else {
        TIRO_ERROR("Expected a number in milliseconds.");
    }

    auto timer = make_ref<Timer>(ctx.io_context());
    timer->timeout_in(millis);
    timer->wait([frame = std::move(frame)](std::error_code ec) mutable {
        if (ec) {
            TIRO_ERROR("Timeout failed.");
        }
        return frame.result(Value::null());
    });
}

static void to_utf8(NativeFunction::Frame& frame) {
    Context& ctx = frame.ctx();
    Handle param = frame.arg(0);
    if (!param->is<String>()) {
        TIRO_ERROR("to_utf8() requires a string argument.");
    }

    Handle string = param.cast<String>();

    Root<Buffer> buffer(
        ctx, Buffer::make(ctx, string->size(), Buffer::uninitialized));

    // Strings are always utf8 encoded.
    std::copy_n(string->data(), string->size(), buffer->data());
    frame.result(buffer);
}

Module create_std_module(Context& ctx) {
    ModuleBuilder builder(ctx, "std");

    builder.add_function("print", 0, {}, print)
        .add_function("new_string_builder", 0, {}, new_string_builder)
        .add_function("new_object", 0, {}, new_object)
        .add_function("new_buffer", 1, {}, new_buffer)
        .add_function("launch", 1, {}, launch)
        .add_function("loop_timestamp", 0, {}, loop_timestamp)
        .add_async_function("sleep", 1, {}, sleep)
        .add_function("to_utf8", 1, {}, to_utf8);
    return builder.build();
}

} // namespace tiro::vm
