#include "vm/objects/primitives.hpp"

#include "vm/context.hpp"
#include "vm/objects/factory.hpp"

namespace tiro::vm {

Undefined Undefined::make(Context& ctx) {
    Layout* data = create_object<Undefined>(ctx);
    return Undefined(from_heap(data));
}

Boolean Boolean::make(Context& ctx, bool value) {
    Layout* data = create_object<Boolean>(ctx, StaticPayloadInit());
    data->static_payload()->value = value;
    return Boolean(from_heap(data));
}

bool Boolean::value() {
    return layout()->static_payload()->value;
}

Integer Integer::make(Context& ctx, i64 value) {
    Layout* data = create_object<Integer>(ctx, StaticPayloadInit());
    data->static_payload()->value = value;
    return Integer(from_heap(data));
}

i64 Integer::value() {
    return layout()->static_payload()->value;
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

i64 SmallInteger::value() {
    TIRO_DEBUG_ASSERT(is_embedded_integer(), "Value does not contain an embedded integer.");

    uintptr_t raw_value = raw();
    raw_value >>= embedded_integer_shift;
    return raw_value <= max ? i64(raw_value) : -i64(raw_value - max);
}

Float Float::make(Context& ctx, f64 value) {
    Layout* data = create_object<Float>(ctx, StaticPayloadInit());
    data->static_payload()->value = value;
    return Float(from_heap(data));
}

f64 Float::value() {
    return layout()->static_payload()->value;
}

Symbol Symbol::make(Context& ctx, Handle<String> name) {
    TIRO_CHECK(!name->is_null(), "The symbol name must be a valid string.");

    Layout* data = create_object<Symbol>(ctx, StaticSlotsInit());
    data->write_static_slot(NameSlot, name);
    return Symbol(from_heap(data));
}

String Symbol::name() {
    return layout()->read_static_slot<String>(NameSlot);
}

bool Symbol::equal(Symbol other) {
    return same(other);
}

} // namespace tiro::vm
