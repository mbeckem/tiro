#include "compiler/ir_gen/const_eval.hpp"

#include "common/safe_int.hpp"

#include <cmath>
#include <limits>

namespace tiro {

std::string_view to_string(EvalResultType type) {
    switch (type) {
#define TIRO_CASE(X)        \
    case EvalResultType::X: \
        return #X;

        TIRO_CASE(Value)
        TIRO_CASE(IntegerOverflow)
        TIRO_CASE(DivideByZero)
        TIRO_CASE(NegativeShift)
        TIRO_CASE(ImaginaryPower)
        TIRO_CASE(TypeError)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid eval result type.");
}

EvalResult::EvalResult(const Constant& value)
    : type_(EvalResultType::Value)
    , value_(value) {}

EvalResult EvalResult::make_integer_overflow() {
    return EvalResult(EvalResultType::IntegerOverflow);
}

EvalResult EvalResult::make_divide_by_zero() {
    return EvalResult(EvalResultType::DivideByZero);
}

EvalResult EvalResult::make_negative_shift() {
    return EvalResult(EvalResultType::NegativeShift);
}

EvalResult EvalResult::make_imaginary_power() {
    return EvalResult(EvalResultType::ImaginaryPower);
}

EvalResult EvalResult::make_type_error() {
    return EvalResult(EvalResultType::TypeError);
}

EvalResult EvalResult::make_error(EvalResultType error) {
    return EvalResult(error);
}

EvalResult::EvalResult(EvalResultType type)
    : type_(type) {
    TIRO_DEBUG_ASSERT(type != EvalResultType::Value, "Result type must represent an error.");
}

void EvalResult::format(FormatStream& stream) const {
    if (is_error()) {
        stream.format("EvalResult({})", type());
        return;
    }
    stream.format("EvalResult({})", value());
}

static constexpr i64 max_shift = 63;

static bool is_integer(const Constant& c) {
    return c.type() == ConstantType::Integer;
}

static bool is_float(const Constant& c) {
    return c.type() == ConstantType::Float;
}

static bool is_numeric(const Constant& c) {
    return is_integer(c) || is_float(c);
}

static i64 get_int(const Constant& c) {
    return c.as_integer().value;
}

static f64 get_float(const Constant& c) {
    return c.as_float().value;
}

static f64 convert_float(const Constant& c) {
    TIRO_DEBUG_ASSERT(is_numeric(c), "Constant must be numeric.");
    return is_integer(c) ? get_int(c) : get_float(c);
}

static i64 to_signed(u64 v) {
    using limits = std::numeric_limits<i64>;
    if (v <= u64(limits::max()))
        return static_cast<i64>(v);

    return static_cast<i64>(v - static_cast<u64>(limits::min())) + limits::min();
}

static bool i64_f64_equal(i64 lhs, f64 rhs) {
    if (!std::isfinite(rhs))
        return false;

    // Check whether converting the float to int and back preserves the value.
    // If that is the case, the two integers can be equal.
    i64 rhs_int = rhs;
    f64 rhs_roundtrip = rhs_int;
    return rhs_roundtrip == rhs && lhs == rhs_int;
}

static bool is_equal(const Constant& lhs, const Constant& rhs) {
    if (is_integer(lhs) && is_float(rhs))
        return i64_f64_equal(get_int(lhs), get_float(rhs));

    if (is_float(lhs) && is_integer(rhs))
        return i64_f64_equal(get_int(rhs), get_float(lhs));

    if (is_float(lhs) && is_float(rhs)) {
        if (std::isnan(get_float(lhs)) && std::isnan(get_float(rhs)))
            return false;
    }

    return lhs == rhs;
}

static Constant make_int(i64 value) {
    return Constant::make_integer(value);
}

static Constant make_float(f64 value) {
    return Constant::make_float(value);
}

static Constant make_bool(bool value) {
    return value ? Constant::make_true() : Constant::make_false();
}

template<typename IntOp, typename FloatOp>
static EvalResult
numeric_op(const Constant& lhs, const Constant& rhs, IntOp&& intop, FloatOp&& floatop) {
    if (!is_numeric(lhs) || !is_numeric(rhs))
        return EvalResult::make_type_error();

    if (is_integer(lhs) && is_integer(rhs)) {
        return intop(get_int(lhs), get_int(rhs));
    } else {
        return floatop(convert_float(lhs), convert_float(rhs));
    }
}

template<typename IntOp>
static EvalResult integer_op(const Constant& lhs, const Constant& rhs, IntOp&& op) {
    if (!is_integer(lhs) || !is_integer(rhs))
        return EvalResult::make_type_error();

    return op(get_int(lhs), get_int(rhs));
}

static EvalResult eval_plus(const Constant& lhs, const Constant& rhs) {
    constexpr auto intop = [](i64 a, i64 b) -> EvalResult {
        SafeInt result = a;
        if (!result.try_add(b))
            return EvalResult::make_integer_overflow();
        return make_int(result.value());
    };
    constexpr auto floatop = [](f64 a, f64 b) -> EvalResult { return make_float(a + b); };
    return numeric_op(lhs, rhs, intop, floatop);
}

static EvalResult eval_minus(const Constant& lhs, const Constant& rhs) {
    constexpr auto intop = [](i64 a, i64 b) -> EvalResult {
        SafeInt result = a;
        if (!result.try_sub(b))
            return EvalResult::make_integer_overflow();
        return make_int(result.value());
    };
    static constexpr auto floatop = [](f64 a, f64 b) -> EvalResult { return make_float(a - b); };
    return numeric_op(lhs, rhs, intop, floatop);
}

static EvalResult eval_multiply(const Constant& lhs, const Constant& rhs) {
    constexpr auto intop = [](i64 a, i64 b) -> EvalResult {
        SafeInt result = a;
        if (!result.try_mul(b))
            return EvalResult::make_integer_overflow();
        return make_int(result.value());
    };
    constexpr auto floatop = [](f64 a, f64 b) -> EvalResult { return make_float(a * b); };
    return numeric_op(lhs, rhs, intop, floatop);
}

static EvalResult eval_divide(const Constant& lhs, const Constant& rhs) {
    static constexpr auto intop = [](i64 a, i64 b) -> EvalResult {
        if (b == 0)
            return EvalResult::make_divide_by_zero();

        SafeInt result = a;
        if (!result.try_div(b))
            return EvalResult::make_integer_overflow();
        return make_int(result.value());
    };
    static constexpr auto floatop = [](f64 a, f64 b) -> EvalResult { return make_float(a / b); };
    return numeric_op(lhs, rhs, intop, floatop);
}

static EvalResult eval_remainder(const Constant& lhs, const Constant& rhs) {
    static constexpr auto intop = [](i64 a, i64 b) -> EvalResult {
        if (b == 0)
            return EvalResult::make_divide_by_zero();

        SafeInt result = a;
        if (!result.try_mod(b))
            return EvalResult::make_integer_overflow();
        return make_int(result.value());
    };
    static constexpr auto floatop = [](f64 a, f64 b) -> EvalResult {
        return make_float(std::fmod(a, b));
    };
    return numeric_op(lhs, rhs, intop, floatop);
}

static EvalResult eval_power(const Constant& lhs, const Constant& rhs) {
    constexpr auto intop = [](i64 a, i64 b) -> EvalResult {
        if (a == 0 && b < 0)
            return EvalResult::make_divide_by_zero();
        if (b < 0)
            return make_int(a == 1 || a == -1 ? a : 0);

        SafeInt result = i64(1);
        while (b != 0) {
            if (!result.try_mul(a))
                return EvalResult::make_integer_overflow();
            b -= 1;
        }
        return make_int(result.value());
    };
    constexpr auto floatop = [](f64 a, f64 b) -> EvalResult { return make_float(std::pow(a, b)); };
    return numeric_op(lhs, rhs, intop, floatop);
}

static EvalResult eval_left_shift(const Constant& lhs, const Constant& rhs) {
    return integer_op(lhs, rhs, [&](i64 a, i64 b) -> EvalResult {
        if (b < 0)
            return EvalResult::make_negative_shift();
        if (b > max_shift)
            return EvalResult::make_integer_overflow();

        u64 result = u64(a) << u64(b);
        return make_int(to_signed(result));
    });
}

static EvalResult eval_right_shift(const Constant& lhs, const Constant& rhs) {
    return integer_op(lhs, rhs, [&](i64 a, i64 b) -> EvalResult {
        if (b < 0)
            return EvalResult::make_negative_shift();
        if (b > max_shift)
            return EvalResult::make_integer_overflow();

        u64 result = u64(a) >> u64(b);
        return make_int(to_signed(result));
    });
}

static EvalResult eval_bitwise_and(const Constant& lhs, const Constant& rhs) {
    return integer_op(lhs, rhs, [&](i64 a, i64 b) { return make_int(a & b); });
}

static EvalResult eval_bitwise_or(const Constant& lhs, const Constant& rhs) {
    return integer_op(lhs, rhs, [&](i64 a, i64 b) { return make_int(a | b); });
}

static EvalResult eval_bitwise_xor(const Constant& lhs, const Constant& rhs) {
    return integer_op(lhs, rhs, [&](i64 a, i64 b) { return make_int(a ^ b); });
}

static EvalResult eval_equals(const Constant& lhs, const Constant& rhs) {
    return make_bool(is_equal(lhs, rhs));
}

static EvalResult eval_not_equals(const Constant& lhs, const Constant& rhs) {
    return make_bool(!is_equal(lhs, rhs));
}

template<typename Test>
static EvalResult numeric_compare(const Constant& lhs, const Constant& rhs, Test&& test) {

    // First, rule out invalid types: inequality is defined for numeric values only.
    if (!is_numeric(lhs) || !is_numeric(rhs))
        return EvalResult::make_type_error();

    // Equality is special (do not contradict operators == and !=).
    if (is_equal(lhs, rhs)) {
        return make_bool(test(0));
    }

    // Apply normal int / float promotion rules.
    auto op = [&](auto a, auto b) -> EvalResult {
        return make_bool(test(a < b ? -1 : (a > b ? 1 : 0)));
    };
    return numeric_op(lhs, rhs, op, op);
}

static EvalResult eval_less(const Constant& lhs, const Constant& rhs) {
    return numeric_compare(lhs, rhs, [](int cmp) { return cmp < 0; });
}

static EvalResult eval_less_equals(const Constant& lhs, const Constant& rhs) {
    return numeric_compare(lhs, rhs, [](int cmp) { return cmp <= 0; });
}

static EvalResult eval_greater(const Constant& lhs, const Constant& rhs) {
    return numeric_compare(lhs, rhs, [](int cmp) { return cmp > 0; });
}

static EvalResult eval_greater_equals(const Constant& lhs, const Constant& rhs) {
    return numeric_compare(lhs, rhs, [](int cmp) { return cmp >= 0; });
}

static EvalResult eval_plus(const Constant& value) {
    if (!is_numeric(value))
        return EvalResult::make_type_error();

    return value;
}

static EvalResult eval_minus(const Constant& value) {
    if (!is_numeric(value))
        return EvalResult::make_type_error();

    if (is_integer(value)) {
        SafeInt result = get_int(value);
        // The result is not defined for all integer values,
        // e.g. -INT_MIN can overflow.
        if (!result.try_mul(-1))
            return EvalResult::make_integer_overflow();

        return make_int(result.value());
    } else {
        return make_float(-get_float(value));
    }
}

static EvalResult eval_bitwise_not(const Constant& value) {
    if (!is_integer(value))
        return EvalResult::make_type_error();

    return make_int(~get_int(value));
}

static EvalResult eval_logical_not(const Constant& value) {
    return make_bool(value.type() == ConstantType::Null || value.type() == ConstantType::False);
}

EvalResult eval_binary_operation(BinaryOpType op, const Constant& lhs, const Constant& rhs) {
    switch (op) {
#define TIRO_CASE(optype, func) \
    case BinaryOpType::optype:  \
        return func(lhs, rhs);

        TIRO_CASE(Plus, eval_plus)
        TIRO_CASE(Minus, eval_minus)
        TIRO_CASE(Multiply, eval_multiply)
        TIRO_CASE(Divide, eval_divide)
        TIRO_CASE(Modulus, eval_remainder)
        TIRO_CASE(Power, eval_power)
        TIRO_CASE(LeftShift, eval_left_shift)
        TIRO_CASE(RightShift, eval_right_shift)
        TIRO_CASE(BitwiseAnd, eval_bitwise_and)
        TIRO_CASE(BitwiseOr, eval_bitwise_or)
        TIRO_CASE(BitwiseXor, eval_bitwise_xor)
        TIRO_CASE(Equals, eval_equals)
        TIRO_CASE(NotEquals, eval_not_equals)
        TIRO_CASE(Less, eval_less)
        TIRO_CASE(LessEquals, eval_less_equals)
        TIRO_CASE(Greater, eval_greater)
        TIRO_CASE(GreaterEquals, eval_greater_equals)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid binary operation.");
}

EvalResult eval_unary_operation(UnaryOpType op, const Constant& value) {
    switch (op) {
#define TIRO_CASE(optype, func) \
    case UnaryOpType::optype:   \
        return func(value);

        TIRO_CASE(Plus, eval_plus)
        TIRO_CASE(Minus, eval_minus)
        TIRO_CASE(BitwiseNot, eval_bitwise_not)
        TIRO_CASE(LogicalNot, eval_logical_not)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid unary operation.");
}

EvalResult eval_format(Span<const Constant> operands, StringTable& strings) {
    struct Visitor {
        StringFormatStream& stream;
        StringTable& strings;

        void visit_integer(const Constant::Integer& i) { stream.format("{}", i.value); }

        void visit_float(const Constant::Float& f) {
            // TODO: need precise format specifier (same format used at runtime)
            stream.format("{}", f.value);
        }

        void visit_string(const Constant::String& s) {
            stream.format("{}", strings.value(s.value));
        }

        void visit_symbol(const Constant::Symbol& s) {
            stream.format("#{}", strings.value(s.value));
        }

        void visit_null(const Constant::Null&) { stream.format("null"); }

        void visit_true(const Constant::True&) { stream.format("true"); }

        void visit_false(const Constant::False&) { stream.format("false"); }
    };

    // TODO alloc parameter?
    StringFormatStream stream;
    {
        Visitor vis{stream, strings};
        for (const auto& op : operands) {
            op.visit(vis);
        }
    }

    std::string result = stream.take_str();
    return Constant::make_string(strings.insert(result));
}

} // namespace tiro
