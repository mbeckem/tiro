#include "hammer/vm/objects/small_integer.hpp"

namespace hammer::vm {

/*
 * Integers in range of [SmallInteger::min, SmallInteger::max] are packed
 * into Value::embedded_integer_bits numbers.
 * embedded_values is the total number of available (unsigned) integer values.
 * 
 * Values in [0, max] are taken as-is. Values in [min, 0) take up the space in
 * (max, embedded_values).
 */
static constexpr uintptr_t embedded_values = (uintptr_t(1)
                                              << Value::embedded_integer_bits);
static_assert(uintptr_t(SmallInteger::max) + uintptr_t(-SmallInteger::min) + 1
                  == embedded_values,
    "Sufficient space to map all values");

SmallInteger SmallInteger::make(i64 value) {
    HAMMER_CHECK(value >= min && value <= max,
        "Value is out of bounds for small integers.");

    uintptr_t raw_value = value >= 0 ? uintptr_t(value)
                                     : uintptr_t(max - value);
    raw_value <<= embedded_integer_shift;
    raw_value |= embedded_integer_flag;
    return SmallInteger(from_embedded_integer(raw_value));
}

i64 SmallInteger::value() const {
    HAMMER_ASSERT(
        is_embedded_integer(), "Value does not contain an embedded integer.");

    uintptr_t raw_value = raw();
    raw_value >>= embedded_integer_shift;
    return raw_value <= max ? i64(raw_value) : -i64(raw_value - max);
}

} // namespace hammer::vm
