#include "vm/objects/primitives.hpp"

#include "vm/context.hpp"
#include "vm/object_support/factory.hpp"

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

HeapInteger HeapInteger::make(Context& ctx, i64 value) {
    Layout* data = create_object<HeapInteger>(ctx, StaticPayloadInit());
    data->static_payload()->value = value;
    return HeapInteger(from_heap(data));
}

i64 HeapInteger::value() {
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
    TIRO_DEBUG_ASSERT(value >= min && value <= max, "value is out of bounds for small integers");

    uintptr_t raw_value = value >= 0 ? uintptr_t(value) : uintptr_t(max - value);
    raw_value <<= embedded_integer_shift;
    raw_value |= embedded_integer_flag;
    return SmallInteger(from_embedded_integer(raw_value));
}

i64 SmallInteger::value() {
    TIRO_DEBUG_ASSERT(is_embedded_integer(), "value does not contain an embedded integer");

    uintptr_t raw_value = raw();
    raw_value >>= embedded_integer_shift;
    return raw_value <= max ? i64(raw_value) : -i64(raw_value - max);
}

template<typename Func>
auto Integer::dispatch(Func&& fn) {
    TIRO_DEBUG_ASSERT(
        is<SmallInteger>() || is<HeapInteger>(), "unexpected type of object in integer");
    if (is<SmallInteger>())
        return fn(SmallInteger(*this));
    if (is<HeapInteger>())
        return fn(HeapInteger(*this));
    TIRO_UNREACHABLE("Invalid integer type");
}

i64 Integer::value() {
    return dispatch([](auto&& v) { return v.value(); });
}

std::optional<size_t> Integer::try_extract_size() {
    i64 i = value();
    if (i < 0)
        return {};

    u64 u = static_cast<u64>(i);
    if (u > std::numeric_limits<size_t>::max())
        return {};

    return static_cast<size_t>(u);
}

Float Float::make(Context& ctx, f64 value) {
    Layout* data = create_object<Float>(ctx, StaticPayloadInit());
    data->static_payload()->value = value;
    return Float(from_heap(data));
}

f64 Float::value() {
    return layout()->static_payload()->value;
}

f64 Number::convert_float() {
    return visit([](auto&& v) { return static_cast<f64>(v.value()); });
}

i64 Number::convert_int() {
    return visit([](auto&& v) { return static_cast<i64>(v.value()); });
}

std::optional<i64> Number::try_extract_int() {
    struct Visitor {
        std::optional<i64> operator()(Integer i) { return i.value(); }
        std::optional<i64> operator()(Float) { return {}; }
    };
    return visit(Visitor());
}

std::optional<size_t> Number::try_extract_size() {
    struct Visitor {
        std::optional<size_t> operator()(Integer i) { return i.try_extract_size(); }
        std::optional<size_t> operator()(Float) { return {}; }
    };
    return visit(Visitor());
}

Number::Which Number::which() {
    TIRO_DEBUG_ASSERT(is<Float>() || is<Integer>(), "Unexpected type of object in number.");

    if (is<Integer>())
        return Which::Integer;
    if (is<Float>())
        return Which::Float;
    TIRO_UNREACHABLE("Invalid number type");
}

Symbol Symbol::make(Context& ctx, Handle<String> name) {
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
