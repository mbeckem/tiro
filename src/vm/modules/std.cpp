#include "vm/modules/modules.hpp"

#include "common/memory/ref_counted.hpp"
#include "vm/modules/module_builder.hpp"
#include "vm/objects/buffer.hpp"
#include "vm/objects/class.hpp"
#include "vm/objects/exception.hpp"
#include "vm/objects/record.hpp"
#include "vm/objects/result.hpp"
#include "vm/objects/string.hpp"

#include "vm/context.ipp"
#include "vm/math.hpp"

#include <cstdio>
#include <limits>

namespace tiro::vm {

namespace {

struct ExposedType {
    std::string_view name;
    PublicType type;
};

} // namespace

static void type_of(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    Handle object = frame.arg(0);
    frame.return_value(ctx.types().type_of(object));
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

    std::string_view message = builder->view();

    const auto& print_impl = ctx.settings().print_stdout;
    if (print_impl)
        print_impl(message);
}

static void new_string_builder(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    frame.return_value(StringBuilder::make(ctx));
}

static void new_buffer(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();

    auto size_arg = frame.arg(0);
    if (!size_arg->is<Integer>()) {
        TIRO_ERROR("Buffer: size should be an integer");
    }

    auto size = size_arg.must_cast<Integer>()->try_extract_size();
    if (!size) {
        TIRO_ERROR("Buffer: size out of bounds.");
    }

    frame.return_value(Buffer::make(ctx, *size, 0));
}

// TODO: Temporary API because we don't have syntax support for records yet.
static void new_record(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    auto maybe_array = frame.arg(0).try_cast<Array>();
    if (!maybe_array)
        TIRO_ERROR("Argument to new_record must be an array.");
    frame.return_value(Record::make(ctx, maybe_array.handle()));
}

static void new_success(NativeFunctionFrame& frame) {
    frame.return_value(Result::make_success(frame.ctx(), frame.arg(0)));
}

static void new_failure(NativeFunctionFrame& frame) {
    frame.return_value(Result::make_failure(frame.ctx(), frame.arg(0)));
}

static void current_coroutine(NativeFunctionFrame& frame) {
    frame.return_value(*frame.coro());
}

static void launch(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    Handle func = frame.arg(0);

    // Rooted on the call site
    auto raw_args = frame.args().raw_slots().drop_front(1);

    Scope sc(ctx);
    Local args = sc.local(Tuple::make(ctx, HandleSpan<Value>(raw_args)));
    Local coro = sc.local(ctx.make_coroutine(func, args));
    ctx.start(coro);
    frame.return_value(*coro);
}

static void loop_timestamp(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    frame.return_value(ctx.get_integer(ctx.loop_timestamp()));
}

static void coroutine_token(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    frame.return_value(Coroutine::create_token(ctx, frame.coro()));
}

static void yield_coroutine(NativeFunctionFrame& frame) {
    frame.coro()->state(CoroutineState::Waiting);
}

static void dispatch(NativeFunctionFrame& frame) {
    Coroutine::schedule(frame.ctx(), frame.coro());
}

static void panic(NativeFunctionFrame& frame) {
    if (frame.arg_count() < 1)
        TIRO_ERROR("panic() requires at least one argument.");

    Context& ctx = frame.ctx();
    Scope sc(ctx);

    auto arg = frame.arg(0);
    if (auto ex = arg.try_cast<Exception>()) {
        return frame.panic(*ex.handle());
    }

    // TODO: Simple to_string() function
    Local message = sc.local<String>(defer_init);
    if (auto message_str = arg.try_cast<String>()) {
        message = message_str.handle();
    } else {
        Local builder = sc.local(StringBuilder::make(ctx));
        to_string(ctx, builder, frame.arg(0));
        message = sc.local(builder->to_string(ctx));
    }

    frame.panic(Exception::make(ctx, message));
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
    frame.return_value(*buffer);
}

static constexpr ExposedType exposed_types[] = {
    {"Array"sv, PublicType::Array},
    {"Boolean"sv, PublicType::Boolean},
    {"Buffer"sv, PublicType::Buffer},
    {"Coroutine"sv, PublicType::Coroutine},
    {"CoroutineToken"sv, PublicType::CoroutineToken},
    {"Exception"sv, PublicType::Exception},
    {"Float"sv, PublicType::Float},
    {"Function"sv, PublicType::Function},
    {"Map"sv, PublicType::Map},
    {"MapKeyView"sv, PublicType::MapKeyView},
    {"MapValueView"sv, PublicType::MapValueView},
    {"Integer"sv, PublicType::Integer},
    {"Module"sv, PublicType::Module},
    {"NativeObject"sv, PublicType::NativeObject},
    {"NativePointer"sv, PublicType::NativePointer},
    {"Null"sv, PublicType::Null},
    {"Record"sv, PublicType::Record},
    {"Result"sv, PublicType::Result},
    {"Set"sv, PublicType::Set},
    {"String"sv, PublicType::String},
    {"StringBuilder"sv, PublicType::StringBuilder},
    {"StringSlice"sv, PublicType::StringSlice},
    {"Symbol"sv, PublicType::Symbol},
    {"Tuple"sv, PublicType::Tuple},
    {"Type"sv, PublicType::Type},
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

        value = MagicFunction::make(ctx, MagicFunction::Catch);
        builder.add_member("catch_panic", value);
    }

    builder //
        .add_function("type_of", 1, {}, NativeFunctionArg::static_sync<type_of>())
        .add_function("print", 0, {}, NativeFunctionArg::static_sync<print>())
        .add_function(
            "new_string_builder", 0, {}, NativeFunctionArg::static_sync<new_string_builder>())
        .add_function("new_buffer", 1, {}, NativeFunctionArg::static_sync<new_buffer>())
        .add_function("new_record", 1, {}, NativeFunctionArg::static_sync<new_record>())
        .add_function("success", 1, {}, NativeFunctionArg::static_sync<new_success>())
        .add_function("failure", 1, {}, NativeFunctionArg::static_sync<new_failure>())
        .add_function("launch", 1, {}, NativeFunctionArg::static_sync<launch>())
        .add_function(
            "current_coroutine", 0, {}, NativeFunctionArg::static_sync<current_coroutine>())
        .add_function("loop_timestamp", 0, {}, NativeFunctionArg::static_sync<loop_timestamp>())
        .add_function("coroutine_token", 0, {}, NativeFunctionArg::static_sync<coroutine_token>())
        .add_function("yield_coroutine", 0, {}, NativeFunctionArg::static_sync<yield_coroutine>())
        .add_function("dispatch", 0, {}, NativeFunctionArg::static_sync<dispatch>())
        .add_function("panic", 1, {}, NativeFunctionArg::static_sync<panic>())
        .add_function("to_utf8", 1, {}, NativeFunctionArg::static_sync<to_utf8>());
    return builder.build();
}

} // namespace tiro::vm
