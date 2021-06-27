#include "vm/math.hpp"

#include "vm/context.hpp"
#include "vm/objects/exception.hpp"
#include "vm/objects/primitives.hpp"
#include "vm/type_system.hpp"

namespace tiro::vm {

namespace {

struct add_op {
    static constexpr std::string_view name = "+"sv;

    Fallible<i64> ints(Context& ctx, i64 a, i64 b) {
        i64 result;
        if (TIRO_UNLIKELY(!checked_add(a, b, result)))
            return TIRO_FORMAT_EXCEPTION(ctx, "integer overflow in addition");
        return result;
    }

    f64 floats(Context&, f64 a, f64 b) { return a + b; }
};

struct sub_op {
    static constexpr std::string_view name = "-"sv;

    Fallible<i64> ints(Context& ctx, i64 a, i64 b) {
        i64 result;
        if (TIRO_UNLIKELY(!checked_sub(a, b, result)))
            return TIRO_FORMAT_EXCEPTION(ctx, "integer overflow in subtraction");
        return result;
    }

    f64 floats(Context&, f64 a, f64 b) { return a - b; }
};

struct mul_op {
    static constexpr std::string_view name = "*"sv;

    Fallible<i64> ints(Context& ctx, i64 a, i64 b) {
        i64 result;
        if (TIRO_UNLIKELY(!checked_mul(a, b, result)))
            return TIRO_FORMAT_EXCEPTION(ctx, "integer overflow in multiplication");
        return result;
    }

    f64 floats(Context&, f64 a, f64 b) { return a * b; }
};

struct div_op {
    static constexpr std::string_view name = "/"sv;

    Fallible<i64> ints(Context& ctx, i64 a, i64 b) {
        if (TIRO_UNLIKELY(b == 0))
            return TIRO_FORMAT_EXCEPTION(ctx, "integer division by zero");
        if (TIRO_UNLIKELY(a == std::numeric_limits<i64>::min()) && (b == -1))
            return TIRO_FORMAT_EXCEPTION(ctx, "integer overflow in division");
        return a / b;
    }

    f64 floats(Context&, f64 a, f64 b) { return a / b; }
};

struct mod_op {
    static constexpr std::string_view name = "%"sv;

    Fallible<i64> ints(Context& ctx, i64 a, i64 b) {
        if (TIRO_UNLIKELY(b == 0))
            return TIRO_FORMAT_EXCEPTION(ctx, "integer modulus by zero");
        if (TIRO_UNLIKELY(a == std::numeric_limits<i64>::min()) && (b == -1))
            return TIRO_FORMAT_EXCEPTION(ctx, "integer overflow in modulus");
        return a % b;
    }

    f64 floats(Context&, f64 a, f64 b) { return std::fmod(a, b); }
};

struct pow_op {
    static constexpr std::string_view name = "**"sv;

    Fallible<i64> ints(Context& ctx, i64 a, i64 b) {
        if (TIRO_UNLIKELY(a == 0 && b < 0))
            return TIRO_FORMAT_EXCEPTION(ctx, "cannot raise integer 0 to a negative power");

        if (b < 0)
            return a == 1 || a == -1 ? a : 0;

        // https://stackoverflow.com/a/101613
        i64 result = 1;
        while (1) {
            if (b & 1) {
                if (TIRO_UNLIKELY(!checked_mul(result, a)))
                    return TIRO_FORMAT_EXCEPTION(ctx, "integer overflow in pow");
            }

            b >>= 1;
            if (!b)
                break;

            if (TIRO_UNLIKELY(!checked_mul(a, a)))
                return TIRO_FORMAT_EXCEPTION(ctx, "integer overflow in pow");
        }
        return result;
    }

    f64 floats(Context&, f64 a, f64 b) { return std::pow(a, b); }
};

template<typename Operation>
static Fallible<Number>
binary_op(Context& ctx, Handle<Value> left, Handle<Value> right, Operation&& op) {
    if (TIRO_UNLIKELY(!left->is<Number>()))
        return invalid_operand_type_exception(ctx, op.name, left);
    if (TIRO_UNLIKELY(!right->is<Number>()))
        return invalid_operand_type_exception(ctx, op.name, right);

    auto left_num = left.must_cast<Number>();
    auto right_num = right.must_cast<Number>();

    if (left_num->is<Float>() || right_num->is<Float>()) {
        f64 a = left_num->convert_float();
        f64 b = right_num->convert_float();
        auto r = op.floats(ctx, a, b);
        if constexpr (is_fallible<decltype(r)>) {
            if (r.has_exception())
                return r.exception();
            return static_cast<Number>(Float::make(ctx, r.value()));
        } else {
            return static_cast<Number>(Float::make(ctx, r));
        }
    }

    i64 a = left_num->convert_int();
    i64 b = right_num->convert_int();
    auto r = op.ints(ctx, a, b);
    if constexpr (is_fallible<decltype(r)>) {
        if (r.has_exception())
            return r.exception();
        return static_cast<Number>(ctx.get_integer(r.value()));
    } else {
        return static_cast<Number>(ctx.get_integer(r));
    }
}

} // namespace

Fallible<Number> add(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, add_op());
}

Fallible<Number> sub(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, sub_op());
}

Fallible<Number> mul(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, mul_op());
}

Fallible<Number> div(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, div_op());
}

Fallible<Number> mod(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, mod_op());
}

Fallible<Number> pow(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, pow_op());
}

Fallible<Number> unary_plus([[maybe_unused]] Context& ctx, Handle<Value> v) {
    if (TIRO_UNLIKELY(!v->is<Number>()))
        return invalid_operand_type_exception(ctx, "unary +"sv, v);
    return static_cast<Number>(*v);
}

Fallible<Number> unary_minus(Context& ctx, Handle<Value> v) {
    if (TIRO_UNLIKELY(!v->is<Number>()))
        return invalid_operand_type_exception(ctx, "unary -"sv, v);

    if (v->is<Integer>()) {
        i64 iv = v.must_cast<Integer>()->value();
        if (TIRO_UNLIKELY(iv == -1))
            return TIRO_FORMAT_EXCEPTION(ctx, "integer overflow in unary minus");
        return static_cast<Number>(ctx.get_integer(-iv));
    }
    if (v->is<Float>()) {
        return static_cast<Number>(Float::make(ctx, -v->must_cast<Float>().value()));
    }
    TIRO_UNREACHABLE("Invalid number type");
}

Fallible<Integer> bitwise_not(Context& ctx, Handle<Value> v) {
    if (TIRO_UNLIKELY(!v->is<Integer>()))
        return invalid_operand_type_exception(ctx, "~"sv, v);

    return ctx.get_integer(~v.must_cast<Integer>()->value());
}

template<typename Callback>
static void unwrap_number(Value v, Callback&& cb) {
    switch (v.type()) {
    case ValueType::SmallInteger:
        return cb(v.must_cast<SmallInteger>().value());
    case ValueType::HeapInteger:
        return cb(v.must_cast<HeapInteger>().value());
    case ValueType::Float:
        return cb(v.must_cast<Float>().value());
    default:
        break;
    }
};

Fallible<int> compare(Context& ctx, Handle<Value> a, Handle<Value> b) {
    if (a->is_null()) {
        if (b->is_null())
            return 0;
        return -1;
    }
    if (b->is_null())
        return 1;

    auto cmp = [](auto lhs, auto rhs) {
        if (lhs > rhs)
            return 1;
        if (lhs < rhs)
            return -1;
        return 0;
    };

    std::optional<int> result;
    unwrap_number(
        *a, [&](auto lhs) { unwrap_number(*b, [&](auto rhs) { result = cmp(lhs, rhs); }); });

    if (TIRO_UNLIKELY(!result))
        return comparison_not_defined_exception(ctx, a, b);
    return *result;
}

} // namespace tiro::vm
