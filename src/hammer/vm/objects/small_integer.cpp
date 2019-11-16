#include "hammer/vm/objects/small_integer.hpp"

namespace hammer::vm {

static constexpr uintptr_t embedded_values = (uintptr_t(1)
                                              << Value::embedded_integer_bits);

SmallInteger SmallInteger::make(i64 value) {
    HAMMER_CHECK(value >= min && value <= max,
        "Value is out of bounds for small integers.");

    static_assert(uintptr_t(max) + uintptr_t(-min) + 1 == embedded_values);

    uintptr_t raw_value;
    if (value >= 0) {
        raw_value = uintptr_t(value);
    } else {
        raw_value = uintptr_t(embedded_values) - uintptr_t(-value);
    }
    raw_value = (raw_value << embedded_integer_shift) | embedded_integer_flag;
    return SmallInteger(from_embedded_integer(raw_value));
}

i64 SmallInteger::value() const {
    HAMMER_ASSERT(
        is_embedded_integer(), "Value does not contain an embedded integer.");

    const uintptr_t raw_value = raw() >> embedded_integer_shift;
    if (raw_value <= max) {
        return i64(raw_value);
    } else {
        return -i64(embedded_values - raw_value);
    }
}

} // namespace hammer::vm
