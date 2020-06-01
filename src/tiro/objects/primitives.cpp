#include "tiro/objects/primitives.hpp"

#include "tiro/vm/context.hpp"

#include "tiro/objects/primitives.ipp"

namespace tiro::vm {

Null Null::make(Context&) {
    return Null(Value());
}

Undefined Undefined::make(Context& ctx) {
    Data* data = ctx.heap().create<Data>();
    return Undefined(from_heap(data));
}

Boolean Boolean::make(Context& ctx, bool value) {
    Data* data = ctx.heap().create<Data>(value);
    return Boolean(from_heap(data));
}

bool Boolean::value() const {
    return access_heap<Data>()->value;
}

Integer Integer::make(Context& ctx, i64 value) {
    Data* data = ctx.heap().create<Data>(value);
    return Integer(from_heap(data));
}

i64 Integer::value() const {
    return access_heap<Data>()->value;
}

Float Float::make(Context& ctx, f64 value) {
    Data* data = ctx.heap().create<Data>(value);
    return Float(from_heap(data));
}

f64 Float::value() const {
    return access_heap<Data>()->value;
}

// Integers in range of [SmallInteger::min, SmallInteger::max] are packed
// into Value::embedded_integer_bits numbers.
// embedded_values is the total number of available (unsigned) integer values.
//
// Values in [0, max] are taken as-is. Values in [min, 0) take up the space in
// (max, embedded_values).
static constexpr uintptr_t embedded_values = (uintptr_t(1) << Value::embedded_integer_bits);
static_assert(uintptr_t(SmallInteger::max) + uintptr_t(-SmallInteger::min) + 1 == embedded_values,
    "Sufficient space to map all values");

SmallInteger SmallInteger::make(i64 value) {
    TIRO_CHECK(value >= min && value <= max, "Value is out of bounds for small integers.");

    uintptr_t raw_value = value >= 0 ? uintptr_t(value) : uintptr_t(max - value);
    raw_value <<= embedded_integer_shift;
    raw_value |= embedded_integer_flag;
    return SmallInteger(from_embedded_integer(raw_value));
}

i64 SmallInteger::value() const {
    TIRO_DEBUG_ASSERT(is_embedded_integer(), "Value does not contain an embedded integer.");

    uintptr_t raw_value = raw();
    raw_value >>= embedded_integer_shift;
    return raw_value <= max ? i64(raw_value) : -i64(raw_value - max);
}

} // namespace tiro::vm
