#include "vm/math.hpp"

#include "vm/context.hpp"
#include "vm/objects/primitives.hpp"

namespace tiro::vm {

namespace {

struct add_op {
    i64 operator()(i64 a, i64 b) {
        i64 result;
        if (TIRO_UNLIKELY(!checked_add(a, b, result))) // TODO exception
            TIRO_ERROR("Integer overflow in addition.");
        return result;
    }

    f64 operator()(f64 a, f64 b) { return a + b; }
};

struct sub_op {
    i64 operator()(i64 a, i64 b) {
        i64 result;
        if (TIRO_UNLIKELY(!checked_sub(a, b, result))) // TODO exception
            TIRO_ERROR("Integer overflow in subtraction.");
        return result;
    }

    f64 operator()(f64 a, f64 b) { return a - b; }
};

struct mul_op {
    i64 operator()(i64 a, i64 b) {
        i64 result;
        if (TIRO_UNLIKELY(!checked_mul(a, b, result)))
            TIRO_ERROR("Integer overflow in multiplication.");
        return result;
    }

    f64 operator()(f64 a, f64 b) { return a * b; }
};

struct div_op {
    i64 operator()(i64 a, i64 b) {
        if (TIRO_UNLIKELY(b == 0))
            TIRO_ERROR("Integer division by zero.");
        if (TIRO_UNLIKELY(a == std::numeric_limits<i64>::min()) && (b == -1))
            TIRO_ERROR("Integer overflow in division.");
        return a / b;
    }

    f64 operator()(f64 a, f64 b) { return a / b; }
};

struct mod_op {
    i64 operator()(i64 a, i64 b) {
        if (TIRO_UNLIKELY(b == 0))
            TIRO_ERROR("Integer modulus by zero.");
        if (TIRO_UNLIKELY(a == std::numeric_limits<i64>::min()) && (b == -1))
            TIRO_ERROR("Integer overflow in modulus.");
        return a % b;
    }

    f64 operator()(f64 a, f64 b) { return std::fmod(a, b); }
};

struct pow_op {
    i64 operator()(i64 a, i64 b) {
        if (TIRO_UNLIKELY(a == 0 && b < 0))
            TIRO_ERROR("Cannot raise 0 to a negative power.");

        if (b < 0)
            return a == 1 || a == -1 ? a : 0;

        // https://stackoverflow.com/a/101613
        i64 result = 1;
        while (1) {
            if (b & 1) {
                if (TIRO_UNLIKELY(!checked_mul(result, a)))
                    TIRO_ERROR("Integer overflow in pow.");
            }

            b >>= 1;
            if (!b)
                break;

            if (TIRO_UNLIKELY(!checked_mul(a, a)))
                TIRO_ERROR("Integer overflow in pow.");
        }
        return result;
    }

    f64 operator()(f64 a, f64 b) { return std::pow(a, b); }
};

template<typename Operation>
static Number binary_op(Context& ctx, Handle<Value> left, Handle<Value> right, Operation&& op) {
    // TODO: Exception
    if (TIRO_UNLIKELY(!left->is<Number>()))
        TIRO_ERROR("Invalid left operand type for binary arithmetic operation: {}", left->type());
    if (TIRO_UNLIKELY(!right->is<Number>()))
        TIRO_ERROR("Invalid right operand type for binary arithmetic operation: {}", right->type());

    auto left_num = left.must_cast<Number>();
    auto right_num = right.must_cast<Number>();

    if (left_num->is<Float>() || right_num->is<Float>()) {
        f64 a = left_num->convert_float();
        f64 b = right_num->convert_float();
        return Float::make(ctx, op(a, b));
    }

    i64 a = left_num->convert_int();
    i64 b = right_num->convert_int();
    return ctx.get_integer(op(a, b));
}

} // namespace

Number add(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, add_op());
}

Number sub(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, sub_op());
}

Number mul(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, mul_op());
}

Number div(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, div_op());
}

Number mod(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, mod_op());
}

Number pow(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, pow_op());
}

Number unary_plus([[maybe_unused]] Context& ctx, Handle<Value> v) {
    if (TIRO_UNLIKELY(!v->is<Number>()))
        TIRO_ERROR("Invalid operand type for unary plus: {}.", to_string(v->type()));

    return static_cast<Number>(*v);
}

Number unary_minus(Context& ctx, Handle<Value> v) {
    if (TIRO_UNLIKELY(!v->is<Number>()))
        TIRO_ERROR("Invalid operand type for unary minus: {}.", to_string(v->type()));

    if (v->is<Integer>()) {
        i64 iv = v.must_cast<Integer>()->value();
        if (TIRO_UNLIKELY(iv == -1))
            TIRO_ERROR("Integer overflow in unary minus.");
        return ctx.get_integer(-iv);
    }
    if (v->is<Float>()) {
        return Float::make(ctx, -v->must_cast<Float>().value());
    }
    TIRO_UNREACHABLE("Invalid number type");
}

Integer bitwise_not(Context& ctx, Handle<Value> v) {
    if (TIRO_UNLIKELY(!v->is<Integer>()))
        TIRO_ERROR("Invalid operand type for bitwise not: {}.", to_string(v->type()));

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

int compare_numbers(Value a, Value b) {
    auto cmp = [](auto lhs, auto rhs) {
        if (lhs > rhs)
            return 1;
        if (lhs < rhs)
            return -1;
        return 0;
    };

    std::optional<int> result;
    unwrap_number(
        a, [&](auto lhs) { unwrap_number(b, [&](auto rhs) { result = cmp(lhs, rhs); }); });

    if (TIRO_UNLIKELY(!result)) {
        TIRO_ERROR("Comparisons are not defined for types {} and {}.", to_string(a.type()),
            to_string(b.type()));
    }
    return *result;
}

} // namespace tiro::vm
