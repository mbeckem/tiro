#include "vm/builtins/modules.hpp"

#include "common/memory/ref_counted.hpp"
#include "vm/builtins/dump.hpp"
#include "vm/builtins/module_builder.hpp"
#include "vm/context.hpp"
#include "vm/error_utils.hpp"
#include "vm/math.hpp"
#include "vm/object_support/type_desc.hpp"
#include "vm/objects/buffer.hpp"
#include "vm/objects/class.hpp"
#include "vm/objects/exception.hpp"
#include "vm/objects/record.hpp"
#include "vm/objects/result.hpp"
#include "vm/objects/string.hpp"

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

static Fallible<Handle<Number>> require_number(Context& ctx, std::string_view function_name,
    std::string_view param_name, Handle<Value> param) {
    auto maybe_number = param.try_cast<Number>();
    if (TIRO_UNLIKELY(!maybe_number)) {
        return TIRO_FORMAT_EXCEPTION(ctx, "{}: {} must be a number", function_name, param_name);
    }
    return maybe_number.handle();
}

static Fallible<f64> require_number_as_f64(Context& ctx, std::string_view function_name,
    std::string_view param_name, Handle<Value> param) {
    auto number = require_number(ctx, function_name, param_name, param);
    if (number.has_exception())
        return number.exception();

    return number.value()->convert_float();
}

static void std_type_of(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    Handle object = frame.arg(0);
    frame.return_value(ctx.types().type_of(object));
}

static void std_schema_of(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    auto maybe_record = frame.arg(0).try_cast<Record>();
    if (!maybe_record)
        return frame.panic(TIRO_FORMAT_EXCEPTION(ctx, "schema_of: argument must be a record"));

    frame.return_value(maybe_record.handle()->schema());
}

static void std_print(SyncFrameContext& frame) {
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

static void std_debug_repr(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    Handle object = frame.arg(0);
    bool pretty = frame.arg_count() > 1 ? ctx.is_truthy(frame.arg(1)) : false;
    frame.return_value(dump(ctx, object, pretty));
}

static void std_new_string_builder(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    frame.return_value(StringBuilder::make(ctx));
}

static void std_new_buffer(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();

    auto size_arg = frame.arg(0);
    if (!size_arg->is<Integer>())
        return frame.panic(TIRO_FORMAT_EXCEPTION(ctx, "new_buffer: size must be an integer"));

    auto size = size_arg.must_cast<Integer>()->try_extract_size();
    if (!size)
        return frame.panic(TIRO_FORMAT_EXCEPTION(ctx, "new_buffer: size out of bounds"));

    frame.return_value(Buffer::make(ctx, *size, 0));
}

static void std_new_success(SyncFrameContext& frame) {
    frame.return_value(Result::make_success(frame.ctx(), frame.arg(0)));
}

static void std_new_error(SyncFrameContext& frame) {
    frame.return_value(Result::make_error(frame.ctx(), frame.arg(0)));
}

static void std_current_coroutine(SyncFrameContext& frame) {
    frame.return_value(*frame.coro());
}

static void std_launch(SyncFrameContext& frame) {
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

static void std_loop_timestamp(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    frame.return_value(ctx.get_integer(ctx.loop_timestamp()));
}

static void std_coroutine_token(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    frame.return_value(Coroutine::create_token(ctx, frame.coro()));
}

static void std_yield_coroutine(SyncFrameContext& frame) {
    frame.coro()->state(CoroutineState::Waiting);
}

static void std_dispatch(SyncFrameContext& frame) {
    Coroutine::schedule(frame.ctx(), frame.coro());
}

static void std_panic(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    if (frame.arg_count() < 1)
        return frame.panic(TIRO_FORMAT_EXCEPTION(ctx, "panic: requires at least one argument"));

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

    frame.panic(Exception::make(ctx, message, /* skip this frame */ 1));
}

static void std_to_utf8(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    Handle param = frame.arg(0);
    if (!param->is<String>()) {
        return frame.panic(TIRO_FORMAT_EXCEPTION(ctx, "to_utf8: requires a string argument"));
    }

    Handle string = param.must_cast<String>();

    Scope sc(ctx);
    Local buffer = sc.local(Buffer::make(ctx, string->size(), Buffer::uninitialized));

    // Strings are always utf8 encoded.
    std::copy_n(string->data(), string->size(), buffer->data());
    frame.return_value(*buffer);
}

static void std_abs(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    TIRO_FRAME_TRY(x, require_number_as_f64(ctx, "abs"sv, "x"sv, frame.arg(0)));
    frame.return_value(Float::make(ctx, std::abs(x)));
}

static void std_pow(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    TIRO_FRAME_TRY(x, require_number_as_f64(ctx, "pow"sv, "x"sv, frame.arg(0)));
    TIRO_FRAME_TRY(y, require_number_as_f64(ctx, "pow"sv, "y"sv, frame.arg(1)));
    frame.return_value(Float::make(ctx, std::pow(x, y)));
}

static void std_log(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    TIRO_FRAME_TRY(x, require_number_as_f64(ctx, "log"sv, "x"sv, frame.arg(0)));
    frame.return_value(Float::make(ctx, std::log(x)));
}

static void std_sqrt(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    TIRO_FRAME_TRY(x, require_number_as_f64(ctx, "sqrt"sv, "x"sv, frame.arg(0)));
    frame.return_value(Float::make(ctx, std::sqrt(x)));
}

static void std_round(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    TIRO_FRAME_TRY(x, require_number_as_f64(ctx, "round"sv, "x"sv, frame.arg(0)));
    frame.return_value(Float::make(ctx, std::round(x)));
}

static void std_ceil(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    TIRO_FRAME_TRY(x, require_number_as_f64(ctx, "ceil"sv, "x"sv, frame.arg(0)));
    frame.return_value(Float::make(ctx, std::ceil(x)));
}

static void std_floor(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    TIRO_FRAME_TRY(x, require_number_as_f64(ctx, "floor"sv, "x"sv, frame.arg(0)));
    frame.return_value(Float::make(ctx, std::floor(x)));
}

static void std_sin(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    TIRO_FRAME_TRY(x, require_number_as_f64(ctx, "sin"sv, "x"sv, frame.arg(0)));
    frame.return_value(Float::make(ctx, std::sin(x)));
}

static void std_cos(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    TIRO_FRAME_TRY(x, require_number_as_f64(ctx, "cos"sv, "x"sv, frame.arg(0)));
    frame.return_value(Float::make(ctx, std::cos(x)));
}

static void std_tan(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    TIRO_FRAME_TRY(x, require_number_as_f64(ctx, "tan"sv, "x"sv, frame.arg(0)));
    frame.return_value(Float::make(ctx, std::tan(x)));
}

static void std_asin(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    TIRO_FRAME_TRY(x, require_number_as_f64(ctx, "asin"sv, "x"sv, frame.arg(0)));
    frame.return_value(Float::make(ctx, std::asin(x)));
}

static void std_acos(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    TIRO_FRAME_TRY(x, require_number_as_f64(ctx, "acos"sv, "x"sv, frame.arg(0)));
    frame.return_value(Float::make(ctx, std::acos(x)));
}

static void std_atan(SyncFrameContext& frame) {
    Context& ctx = frame.ctx();
    TIRO_FRAME_TRY(x, require_number_as_f64(ctx, "atan"sv, "x"sv, frame.arg(0)));
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
    {"RecordSchema"sv, PublicType::RecordSchema},
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

static constexpr FunctionDesc functions[] = {
    // I/O
    FunctionDesc::plain("print"sv, 0, std_print, FunctionDesc::Variadic),
    FunctionDesc::plain("debug_repr", 1, std_debug_repr),
    FunctionDesc::plain("loop_timestamp"sv, 0, std_loop_timestamp),
    FunctionDesc::plain("to_utf8"sv, 1, std_to_utf8),

    // Math
    FunctionDesc::plain("abs"sv, 1, std_abs),
    FunctionDesc::plain("pow"sv, 2, std_pow),
    FunctionDesc::plain("log"sv, 1, std_log),
    FunctionDesc::plain("sqrt"sv, 1, std_sqrt),
    FunctionDesc::plain("round"sv, 1, std_round),
    FunctionDesc::plain("ceil"sv, 1, std_ceil),
    FunctionDesc::plain("floor"sv, 1, std_floor),
    FunctionDesc::plain("sin"sv, 1, std_sin),
    FunctionDesc::plain("cos"sv, 1, std_cos),
    FunctionDesc::plain("tan"sv, 1, std_tan),
    FunctionDesc::plain("asin"sv, 1, std_asin),
    FunctionDesc::plain("acos"sv, 1, std_acos),
    FunctionDesc::plain("atan"sv, 1, std_atan),

    // Utilities
    FunctionDesc::plain("type_of"sv, 1, std_type_of),
    FunctionDesc::plain("schema_of"sv, 1, std_schema_of),

    // Error handling
    FunctionDesc::plain("success"sv, 1, std_new_success),
    FunctionDesc::plain("error"sv, 1, std_new_error),
    FunctionDesc::plain("panic"sv, 1, std_panic),

    // Coroutines
    FunctionDesc::plain("launch"sv, 1, std_launch),
    FunctionDesc::plain("current_coroutine"sv, 0, std_current_coroutine),
    FunctionDesc::plain("coroutine_token"sv, 0, std_coroutine_token),
    FunctionDesc::plain("yield_coroutine"sv, 0, std_yield_coroutine),
    FunctionDesc::plain("dispatch"sv, 0, std_dispatch),

    // Constructor functions (TODO)
    FunctionDesc::plain("new_string_builder"sv, 0, std_new_string_builder),
    FunctionDesc::plain("new_buffer"sv, 1, std_new_buffer),
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
        builder.add_function(fn.name, fn.params, fn.func);
    }

    return builder.build();
}

} // namespace tiro::vm
