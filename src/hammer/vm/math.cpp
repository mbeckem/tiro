#include "hammer/vm/math.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/objects/primitives.hpp"

namespace hammer::vm {

std::optional<size_t> try_extract_size(Value v) {
    std::optional<i64> integer = try_extract_integer(v);
    if (!integer || *integer < 0)
        return {};

    u64 unsigned_integer = static_cast<u64>(*integer);
    if (unsigned_integer > std::numeric_limits<size_t>::max())
        return {};

    return static_cast<size_t>(unsigned_integer);
}

size_t extract_size(Value v) {
    if (auto opt = try_extract_size(v); HAMMER_LIKELY(opt))
        return *opt;
    HAMMER_ERROR("The given value is not a valid size.");
}

std::optional<i64> try_extract_integer(Value v) {
    switch (v.type()) {
    case ValueType::Integer:
        return v.as<Integer>().value();
    case ValueType::SmallInteger:
        return v.as<SmallInteger>().value();
    default:
        return {};
    }
}

i64 extract_integer(Value v) {
    if (auto opt = try_extract_integer(v); HAMMER_LIKELY(opt))
        return *opt;
    HAMMER_ERROR("Value of type {} is not an integer.", to_string(v.type()));
}

std::optional<i64> try_convert_integer(Value v) {
    switch (v.type()) {
    case ValueType::Integer:
        return v.as<Integer>().value();
    case ValueType::SmallInteger:
        return v.as<SmallInteger>().value();
    case ValueType::Float:
        return v.as<Float>().value();

    default:
        return {};
    }
}

i64 convert_integer(Value v) {
    if (auto opt = try_extract_integer(v); HAMMER_LIKELY(opt))
        return *opt;
    HAMMER_ERROR(
        "Cannot convert value of type {} to integer.", to_string(v.type()));
}

std::optional<f64> try_convert_float(Value v) {
    switch (v.type()) {
    case ValueType::Integer:
        return v.as<Integer>().value();
    case ValueType::SmallInteger:
        return v.as<SmallInteger>().value();
    case ValueType::Float:
        return v.as<Float>().value();

    default:
        return {};
    }
}

f64 convert_float(Value v) {
    if (auto opt = try_convert_float(v); HAMMER_LIKELY(opt))
        return *opt;
    HAMMER_ERROR(
        "Cannot convert value of type {} to float.", to_string(v.type()));
}

namespace {

struct add_op {
    i64 operator()(i64 a, i64 b) {
        i64 result;
        if (HAMMER_UNLIKELY(!checked_add(a, b, result))) // TODO exception
            HAMMER_ERROR("Integer overflow in addition.");
        return result;
    }

    f64 operator()(f64 a, f64 b) { return a + b; }
};

struct sub_op {
    i64 operator()(i64 a, i64 b) {
        i64 result;
        if (HAMMER_UNLIKELY(!checked_sub(a, b, result))) // TODO exception
            HAMMER_ERROR("Integer overflow in subtraction.");
        return result;
    }

    f64 operator()(f64 a, f64 b) { return a - b; }
};

struct mul_op {
    i64 operator()(i64 a, i64 b) {
        i64 result;
        if (HAMMER_UNLIKELY(!checked_mul(a, b, result)))
            HAMMER_ERROR("Integer overflow in multiplication.");
        return result;
    }

    f64 operator()(f64 a, f64 b) { return a * b; }
};

struct div_op {
    i64 operator()(i64 a, i64 b) {
        if (HAMMER_UNLIKELY(b == 0))
            HAMMER_ERROR("Integer division by zero.");
        if (HAMMER_UNLIKELY(a == std::numeric_limits<i64>::min()) && (b == -1))
            HAMMER_ERROR("Integer overflow in division.");
        return a / b;
    }

    f64 operator()(f64 a, f64 b) { return a / b; }
};

struct mod_op {
    i64 operator()(i64 a, i64 b) {
        if (HAMMER_UNLIKELY(b == 0))
            HAMMER_ERROR("Integer modulus by zero.");
        if (HAMMER_UNLIKELY(a == std::numeric_limits<i64>::min()) && (b == -1))
            HAMMER_ERROR("Integer overflow in modulus.");
        return a % b;
    }

    f64 operator()(f64 a, f64 b) { return std::fmod(a, b); }
};

struct pow_op {
    i64 operator()(i64 a, i64 b) {
        // TODO negative integers
        if (HAMMER_UNLIKELY(b < 0)) {
            HAMMER_ERROR("Negative exponents not implemented for integer pow.");
        }

        // TODO Speed up, e.g. recursive algorithm for powers of two :)
        i64 result = 1;
        while (b-- > 0) {
            if (HAMMER_UNLIKELY(!checked_mul(result, a))) {
                HAMMER_ERROR("Integer overflow in pow.");
            }
        }
        return result;
    }

    f64 operator()(f64 a, f64 b) { return std::pow(a, b); }
};

template<typename Operation>
static Value binary_op(
    Context& ctx, Handle<Value> left, Handle<Value> right, Operation&& op) {
    if (left->is<Float>() || right->is<Float>()) {
        f64 a = convert_float(left);
        f64 b = convert_float(right);
        return Float::make(ctx, op(a, b));
    } else {
        i64 a = convert_integer(left);
        i64 b = convert_integer(right);
        return ctx.get_integer(op(a, b));
    }
}

} // namespace

Value add(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, add_op());
}

Value sub(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, sub_op());
}

Value mul(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, mul_op());
}

Value div(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, div_op());
}

Value mod(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, mod_op());
}

Value pow(Context& ctx, Handle<Value> a, Handle<Value> b) {
    return binary_op(ctx, a, b, pow_op());
}

Value unary_plus([[maybe_unused]] Context& ctx, Handle<Value> v) {
    switch (v->type()) {
    case ValueType::Integer:
    case ValueType::SmallInteger:
    case ValueType::Float:
        return v;

    default:
        HAMMER_ERROR(
            "Invalid operand type for unary plus: {}.", to_string(v->type()));
    }
}

Value unary_minus(Context& ctx, Handle<Value> v) {
    switch (v->type()) {
    case ValueType::Integer:
    case ValueType::SmallInteger: {
        i64 iv = extract_integer(v);
        if (HAMMER_UNLIKELY(iv == -1))
            HAMMER_ERROR("Integer overflow in unary minus.");
        return ctx.get_integer(-iv);
    }
    case ValueType::Float:
        return Float::make(ctx, -v->as<Float>().value());
    default:
        HAMMER_ERROR(
            "Invalid operand type for unary minus: {}.", to_string(v->type()));
    }
}

int compare_numbers(Handle<Value> a, Handle<Value> b) {
    auto cmp = [](auto lhs, auto rhs) {
        if (lhs > rhs)
            return 1;
        else if (lhs < rhs)
            return -1;
        return 0;
    };

    auto unwrap = [](Value v, auto&& cb) {
        switch (v.type()) {
        case ValueType::SmallInteger:
            return cb(v.as_strict<SmallInteger>().value());
        case ValueType::Integer:
            return cb(v.as_strict<Integer>().value());
        case ValueType::Float:
            return cb(v.as_strict<Float>().value());
        default:
            break;
        }
    };

    std::optional<int> result;
    unwrap(a.get(), [&](auto lhs) {
        unwrap(b.get(), [&](auto rhs) { result = cmp(lhs, rhs); });
    });

    if (HAMMER_UNLIKELY(!result)) {
        HAMMER_ERROR("Comparisons are not defined for types {} and {}.",
            to_string(a->type()), to_string(b->type()));
    }
    return *result;
}

} // namespace hammer::vm
