#include "vm/modules/modules.hpp"

#include "common/ref_counted.hpp"
#include "vm/modules/module_builder.hpp"
#include "vm/objects/buffer.hpp"
#include "vm/objects/class.hpp"
#include "vm/objects/result.hpp"
#include "vm/objects/string.hpp"

#include "vm/context.ipp"
#include "vm/math.hpp"

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
            [self = Ref(this), cb = std::forward<Callback>(callback)](std::error_code ec) mutable {
                self->in_wait_ = false;
                cb(ec);
            });
    }

private:
    asio::steady_timer timer_;
    bool in_wait_ = false;
};

struct ExposedType {
    std::string_view name;
    ValueType type;
};

} // namespace

static void type_of(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    Handle object = frame.arg(0);
    frame.result(ctx.types().type_of(object));
}

static void print(NativeFunctionFrame& frame) {
    const size_t args = frame.arg_count();

    Context& ctx = frame.ctx();
    Scope sc(ctx);
    Local builder = sc.local(StringBuilder::make(ctx));
    for (size_t i = 0; i < args; ++i) {
        if (i != 0) {
            builder->append(ctx, " ");
        }
        to_string(ctx, builder, frame.arg(i));
    }
    builder->append(ctx, "\n");

    // TODO stdout from settings
    std::string_view message = builder->view();
    std::fwrite(message.data(), 1, message.size(), stdout);
    std::fflush(stdout);
}

static void new_string_builder(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    frame.result(StringBuilder::make(ctx));
}

static void new_object(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    frame.result(DynamicObject::make(ctx));
}

static void new_buffer(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();

    size_t size = 0;
    if (auto arg = try_extract_size(frame.arg(0).get())) {
        size = *arg;
    } else {
        TIRO_ERROR("Invalid size argument for buffer creation.");
    }

    frame.result(Buffer::make(ctx, size, 0));
}

static void new_success(NativeFunctionFrame& frame) {
    frame.result(Result::make_success(frame.ctx(), frame.arg(0)));
}

static void new_failure(NativeFunctionFrame& frame) {
    frame.result(Result::make_failure(frame.ctx(), frame.arg(0)));
}

static void current_coroutine(NativeFunctionFrame& frame) {
    frame.result(*frame.coro());
}

static void launch(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    Handle func = frame.arg(0);

    // Rooted on the call site
    auto raw_args = frame.args().raw_slots().drop_front(1);

    Scope sc(ctx);
    Local args = sc.local(Tuple::make(ctx, HandleSpan<Value>(raw_args)));
    frame.result(ctx.make_coroutine(func, args));
}

static void loop_timestamp(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    frame.result(ctx.get_integer(ctx.loop_timestamp()));
}

static void sleep(NativeAsyncFunctionFrame frame) {
    Context& ctx = frame.ctx();

    i64 millis = 0;
    if (auto extracted = try_convert_integer(*frame.arg(0))) {
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

static void to_utf8(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    Handle param = frame.arg(0);
    if (!param->is<String>()) {
        TIRO_ERROR("to_utf8() requires a string argument.");
    }

    Handle string = param.must_cast<String>();

    Scope sc(ctx);
    Local buffer = sc.local(Buffer::make(ctx, string->size(), Buffer::uninitialized));

    // Strings are always utf8 encoded.
    std::copy_n(string->data(), string->size(), buffer->data());
    frame.result(*buffer);
}

static constexpr ExposedType exposed_types[] = {
    {"Array"sv, ValueType::Array},
    {"Boolean"sv, ValueType::Boolean},
    {"Buffer"sv, ValueType::Buffer},
    {"Coroutine"sv, ValueType::Coroutine},
    {"DynamicObject"sv, ValueType::DynamicObject},
    {"Float"sv, ValueType::Float},
    {"Function"sv, ValueType::Function},
    {"Map"sv, ValueType::HashTable},
    {"MapKeyView"sv, ValueType::HashTableKeyView},
    {"MapValueView"sv, ValueType::HashTableValueView},
    {"Integer"sv, ValueType::Integer},
    {"Module"sv, ValueType::Module},
    {"NativeObject"sv, ValueType::NativeObject},
    {"NativePointer"sv, ValueType::NativePointer},
    {"Null"sv, ValueType::Null},
    {"Result"sv, ValueType::Result},
    {"String"sv, ValueType::String},
    {"StringBuilder"sv, ValueType::StringBuilder},
    {"StringSlice"sv, ValueType::StringSlice},
    {"Symbol"sv, ValueType::Symbol},
    {"Tuple"sv, ValueType::Tuple},
    {"Type", ValueType::Type},
};

Module create_std_module(Context& ctx) {
    ModuleBuilder builder(ctx, "std");

    {
        Scope sc(ctx);
        Local value = sc.local();
        for (const auto& exposed : exposed_types) {
            value = ctx.types().type_of(exposed.type);
            builder.add_member(exposed.name, value);
        }
    }

    builder.add_function("type_of", 1, {}, type_of)
        .add_function("print", 0, {}, print)
        .add_function("new_string_builder", 0, {}, new_string_builder)
        .add_function("new_object", 0, {}, new_object)
        .add_function("new_buffer", 1, {}, new_buffer)
        .add_function("success", 1, {}, new_success)
        .add_function("failure", 1, {}, new_failure)
        .add_function("launch", 1, {}, launch)
        .add_function("current_coroutine", 0, {}, current_coroutine)
        .add_function("loop_timestamp", 0, {}, loop_timestamp)
        .add_async_function("sleep", 1, {}, sleep)
        .add_function("to_utf8", 1, {}, to_utf8);
    return builder.build();
}

} // namespace tiro::vm
