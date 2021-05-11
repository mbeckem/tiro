#include "vm/modules/modules.hpp"

#include "common/memory/ref_counted.hpp"
#include "vm/modules/module_builder.hpp"
#include "vm/object_support/type_desc.hpp"
#include "vm/objects/buffer.hpp"
#include "vm/objects/class.hpp"
#include "vm/objects/exception.hpp"
#include "vm/objects/record.hpp"
#include "vm/objects/result.hpp"
#include "vm/objects/string.hpp"

#include "vm/context.ipp"
#include "vm/math.hpp"

#include <cmath>
#include <cstdio>
#include <limits>

namespace tiro::vm {

namespace {

struct ExposedType {
    std::string_view name;
    PublicType type;
};

struct MathConstant {
    std::string_view name;
    f64 value;
};

} // namespace

static Handle<Number>
require_number(std::string_view function_name, std::string_view param_name, Handle<Value> param) {
    auto maybe_number = param.try_cast<Number>();
    if (TIRO_UNLIKELY(!maybe_number)) {
        // TODO: Exception
        TIRO_ERROR("{}: {} must be a number", function_name, param_name);
    }
    return maybe_number.handle();
}

static f64 require_number_as_f64(
    std::string_view function_name, std::string_view param_name, Handle<Value> param) {
    auto number = require_number(function_name, param_name, param);
    return number->convert_float();
}

static void std_type_of(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    Handle object = frame.arg(0);
    frame.return_value(ctx.types().type_of(object));
}

static void std_print(NativeFunctionFrame& frame) {
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

static void std_new_string_builder(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    frame.return_value(StringBuilder::make(ctx));
}

static void std_new_buffer(NativeFunctionFrame& frame) {
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
static void std_new_record(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    auto maybe_array = frame.arg(0).try_cast<Array>();
    if (!maybe_array)
        TIRO_ERROR("Argument to new_record must be an array.");
    frame.return_value(Record::make(ctx, maybe_array.handle()));
}

static void std_new_success(NativeFunctionFrame& frame) {
    frame.return_value(Result::make_success(frame.ctx(), frame.arg(0)));
}

static void std_new_failure(NativeFunctionFrame& frame) {
    frame.return_value(Result::make_failure(frame.ctx(), frame.arg(0)));
}

static void std_current_coroutine(NativeFunctionFrame& frame) {
    frame.return_value(*frame.coro());
}

static void std_launch(NativeFunctionFrame& frame) {
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

static void std_loop_timestamp(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    frame.return_value(ctx.get_integer(ctx.loop_timestamp()));
}

static void std_coroutine_token(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    frame.return_value(Coroutine::create_token(ctx, frame.coro()));
}

static void std_yield_coroutine(NativeFunctionFrame& frame) {
    frame.coro()->state(CoroutineState::Waiting);
}

static void std_dispatch(NativeFunctionFrame& frame) {
    Coroutine::schedule(frame.ctx(), frame.coro());
}

static void std_panic(NativeFunctionFrame& frame) {
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

static void std_to_utf8(NativeFunctionFrame& frame) {
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

static void std_abs(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    f64 x = require_number_as_f64("abs"sv, "x"sv, frame.arg(0));
    frame.return_value(Float::make(ctx, std::abs(x)));
}

static void std_pow(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    f64 x = require_number_as_f64("pow"sv, "x"sv, frame.arg(0));
    f64 y = require_number_as_f64("pow"sv, "y"sv, frame.arg(1));
    frame.return_value(Float::make(ctx, std::pow(x, y)));
}

static void std_log(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    f64 x = require_number_as_f64("log"sv, "x"sv, frame.arg(0));
    frame.return_value(Float::make(ctx, std::log(x)));
}

static void std_sqrt(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    f64 x = require_number_as_f64("sqrt"sv, "x"sv, frame.arg(0));
    frame.return_value(Float::make(ctx, std::sqrt(x)));
}

static void std_round(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    f64 x = require_number_as_f64("round"sv, "x"sv, frame.arg(0));
    frame.return_value(Float::make(ctx, std::round(x)));
}

static void std_ceil(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    f64 x = require_number_as_f64("ceil"sv, "x"sv, frame.arg(0));
    frame.return_value(Float::make(ctx, std::ceil(x)));
}

static void std_floor(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    f64 x = require_number_as_f64("floor"sv, "x"sv, frame.arg(0));
    frame.return_value(Float::make(ctx, std::floor(x)));
}

static void std_sin(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    f64 x = require_number_as_f64("sin"sv, "x"sv, frame.arg(0));
    frame.return_value(Float::make(ctx, std::sin(x)));
}

static void std_cos(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    f64 x = require_number_as_f64("cos"sv, "x"sv, frame.arg(0));
    frame.return_value(Float::make(ctx, std::cos(x)));
}

static void std_tan(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    f64 x = require_number_as_f64("tan"sv, "x"sv, frame.arg(0));
    frame.return_value(Float::make(ctx, std::tan(x)));
}

static void std_asin(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    f64 x = require_number_as_f64("asin"sv, "x"sv, frame.arg(0));
    frame.return_value(Float::make(ctx, std::asin(x)));
}

static void std_acos(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    f64 x = require_number_as_f64("acos"sv, "x"sv, frame.arg(0));
    frame.return_value(Float::make(ctx, std::acos(x)));
}

static void std_atan(NativeFunctionFrame& frame) {
    Context& ctx = frame.ctx();
    f64 x = require_number_as_f64("atan"sv, "x"sv, frame.arg(0));
    frame.return_value(Float::make(ctx, std::atan(x)));
}

static constexpr ExposedType types[] = {
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

static constexpr MathConstant math_constants[] = {
    // https://en.wikipedia.org/wiki/List_of_mathematical_constants
    {"PI"sv, 3.14159'26535'89793'23846},
    {"TAU"sv, 2 * 3.14159'26535'89793'23846},
    {"E"sv, 2.71828'18284'59045'23536},

    {"INFINITY", std::numeric_limits<f64>::infinity()},
};

static const /* constexpr */ FunctionDesc functions[] = {
    // I/O
    FunctionDesc::plain(
        "print"sv, 0, NativeFunctionArg::static_sync<std_print>(), FunctionDesc::Variadic),
    FunctionDesc::plain(
        "loop_timestamp"sv, 0, NativeFunctionArg::static_sync<std_loop_timestamp>()),
    FunctionDesc::plain("to_utf8"sv, 1, NativeFunctionArg::static_sync<std_to_utf8>()),

    // Math
    FunctionDesc::plain("abs"sv, 1, NativeFunctionArg::static_sync<std_abs>()),
    FunctionDesc::plain("pow"sv, 2, NativeFunctionArg::static_sync<std_pow>()),
    FunctionDesc::plain("log"sv, 1, NativeFunctionArg::static_sync<std_log>()),
    FunctionDesc::plain("sqrt"sv, 1, NativeFunctionArg::static_sync<std_sqrt>()),
    FunctionDesc::plain("round"sv, 1, NativeFunctionArg::static_sync<std_round>()),
    FunctionDesc::plain("ceil"sv, 1, NativeFunctionArg::static_sync<std_ceil>()),
    FunctionDesc::plain("floor"sv, 1, NativeFunctionArg::static_sync<std_floor>()),
    FunctionDesc::plain("sin"sv, 1, NativeFunctionArg::static_sync<std_sin>()),
    FunctionDesc::plain("cos"sv, 1, NativeFunctionArg::static_sync<std_cos>()),
    FunctionDesc::plain("tan"sv, 1, NativeFunctionArg::static_sync<std_tan>()),
    FunctionDesc::plain("asin"sv, 1, NativeFunctionArg::static_sync<std_asin>()),
    FunctionDesc::plain("acos"sv, 1, NativeFunctionArg::static_sync<std_acos>()),
    FunctionDesc::plain("atan"sv, 1, NativeFunctionArg::static_sync<std_atan>()),

    // Utilities
    FunctionDesc::plain("type_of"sv, 1, NativeFunctionArg::static_sync<std_type_of>()),

    // Error handling
    FunctionDesc::plain("success"sv, 1, NativeFunctionArg::static_sync<std_new_success>()),
    FunctionDesc::plain("failure"sv, 1, NativeFunctionArg::static_sync<std_new_failure>()),
    FunctionDesc::plain("panic"sv, 1, NativeFunctionArg::static_sync<std_panic>()),

    // Coroutines
    FunctionDesc::plain("launch"sv, 1, NativeFunctionArg::static_sync<std_launch>()),
    FunctionDesc::plain(
        "current_coroutine"sv, 0, NativeFunctionArg::static_sync<std_current_coroutine>()),
    FunctionDesc::plain(
        "coroutine_token"sv, 0, NativeFunctionArg::static_sync<std_coroutine_token>()),
    FunctionDesc::plain(
        "yield_coroutine"sv, 0, NativeFunctionArg::static_sync<std_yield_coroutine>()),
    FunctionDesc::plain("dispatch"sv, 0, NativeFunctionArg::static_sync<std_dispatch>()),

    // Constructor functions (TODO)
    FunctionDesc::plain(
        "new_string_builder"sv, 0, NativeFunctionArg::static_sync<std_new_string_builder>()),
    FunctionDesc::plain("new_buffer"sv, 1, NativeFunctionArg::static_sync<std_new_buffer>()),
    FunctionDesc::plain("new_record"sv, 1, NativeFunctionArg::static_sync<std_new_record>()),
};

Module create_std_module(Context& ctx) {
    ModuleBuilder builder(ctx, "std"sv);
    Scope sc(ctx);

    {
        Local type_instance = sc.local();
        for (const auto& type : types) {
            type_instance = ctx.types().type_of(type.type);
            builder.add_member(type.name, type_instance);
        }
    }

    {
        Local value = sc.local();
        for (const auto& constant : math_constants) {
            value = Float::make(ctx, constant.value);
            builder.add_member(constant.name, value);
        }
    }

    {
        Local fn = sc.local(MagicFunction::make(ctx, MagicFunction::Catch));
        builder.add_member("catch_panic"sv, fn);
    }

    for (const auto& fn : functions) {
        TIRO_DEBUG_ASSERT((fn.flags & FunctionDesc::InstanceMethod) == 0,
            "Instance methods not supported as module members.");
        builder.add_function(fn.name, fn.params, {}, fn.func);
    }

    return builder.build();
}

} // namespace tiro::vm
