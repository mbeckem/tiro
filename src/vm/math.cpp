#include "vm/math.hpp"

#include "vm/context.hpp"
#include "vm/objects/primitives.hpp"

namespace tiro::vm {

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
    if (auto opt = try_extract_size(v); TIRO_LIKELY(opt))
        return *opt;
    TIRO_ERROR("The given value is not a valid size.");
}

std::optional<i64> try_extract_integer(Value v) {
    switch (v.type()) {
    case ValueType::Integer:
        return v.must_cast<Integer>().value();
    case ValueType::SmallInteger:
        return v.must_cast<SmallInteger>().value();
    default:
        return {};
    }
}

i64 extract_integer(Value v) {
    if (auto opt = try_extract_integer(v); TIRO_LIKELY(opt))
        return *opt;
    TIRO_ERROR("Value of type {} is not an integer.", to_string(v.type()));
}

std::optional<i64> try_convert_integer(Value v) {
    switch (v.type()) {
    case ValueType::Integer:
        return v.must_cast<Integer>().value();
    case ValueType::SmallInteger:
        return v.must_cast<SmallInteger>().value();
    case ValueType::Float:
        return v.must_cast<Float>().value();

    default:
        return {};
    }
}

i64 convert_integer(Value v) {
    if (auto opt = try_extract_integer(v); TIRO_LIKELY(opt))
        return *opt;
    TIRO_ERROR("Cannot convert value of type {} to integer.", to_string(v.type()));
}

std::optional<f64> try_convert_float(Value v) {
    switch (v.type()) {
    case ValueType::Integer:
        return v.must_cast<Integer>().value();
    case ValueType::SmallInteger:
        return v.must_cast<SmallInteger>().value();
    case ValueType::Float:
        return v.must_cast<Float>().value();

    default:
        return {};
    }
}

f64 convert_float(Value v) {
    if (auto opt = try_convert_float(v); TIRO_LIKELY(opt))
        return *opt;
    TIRO_ERROR("Cannot convert value of type {} to float.", to_string(v.type()));
}

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
static Value binary_op(Context& ctx, Handle<Value> left, Handle<Value> right, Operation&& op) {
    if (left->is<Float>() || right->is<Float>()) {
        f64 a = convert_float(*left);
        f64 b = convert_float(*right);
        return Float::make(ctx, op(a, b));
    }

    i64 a = convert_integer(*left);
    i64 b = convert_integer(*right);
    return ctx.get_integer(op(a, b));
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
        return v.get();

    default:
        TIRO_ERROR("Invalid operand type for unary plus: {}.", to_string(v->type()));
    }
}

Value unary_minus(Context& ctx, Handle<Value> v) {
    switch (v->type()) {
    case ValueType::Integer:
    case ValueType::SmallInteger: {
        i64 iv = extract_integer(*v);
        if (TIRO_UNLIKELY(iv == -1))
            TIRO_ERROR("Integer overflow in unary minus.");
        return ctx.get_integer(-iv);
    }
    case ValueType::Float:
        return Float::make(ctx, -v->must_cast<Float>().value());
    default:
        TIRO_ERROR("Invalid operand type for unary minus: {}.", to_string(v->type()));
    }
}

template<typename Callback>
static void unwrap_number(Value v, Callback&& cb) {
    switch (v.type()) {
    case ValueType::SmallInteger:
        return cb(v.must_cast<SmallInteger>().value());
    case ValueType::Integer:
        return cb(v.must_cast<Integer>().value());
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
