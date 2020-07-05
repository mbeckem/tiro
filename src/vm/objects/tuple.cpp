#include "vm/objects/tuple.hpp"

#include "vm/context.hpp"

namespace tiro::vm {

Tuple Tuple::make(Context& ctx, size_t size) {
    return make_impl(ctx, size, [&](Span<Value> tuple_values) {
        TIRO_DEBUG_ASSERT(tuple_values.size() == size, "Unexpected tuple size.");
        std::uninitialized_fill_n(tuple_values.data(), tuple_values.size(), Value::null());
    });
}

Tuple Tuple::make(Context& ctx, Span<const Value> initial_values) {
    return make_impl(ctx, initial_values.size(), [&](Span<Value> tuple_values) {
        TIRO_DEBUG_ASSERT(tuple_values.size() == initial_values.size(), "Unexpected tuple size.");
        std::uninitialized_copy_n(
            initial_values.data(), initial_values.size(), tuple_values.data());
    });
}

Tuple Tuple::make(Context& ctx, Span<const Value> initial_values, size_t size) {
    TIRO_DEBUG_ASSERT(
        size >= initial_values.size(), "Tuple::make(): invalid size, must be >= values.size().");

    const size_t copy = initial_values.size();
    const size_t fill = size - copy;
    return make_impl(ctx, size, [&](Span<Value> tuple_values) {
        TIRO_DEBUG_ASSERT(tuple_values.size() == size, "Unexpected tuple size.");
        Value* pos = std::uninitialized_copy_n(initial_values.data(), copy, tuple_values.data());
        std::uninitialized_fill_n(pos, fill, Value::null());
    });
}

Tuple Tuple::make(Context& ctx, std::initializer_list<Handle<Value>> values) {
    return make_impl(ctx, values.size(), [&](Span<Value> tuple_values) {
        TIRO_DEBUG_ASSERT(tuple_values.size() == values.size(), "Unexpected tuple size.");
        auto pos = values.begin();
        auto end = values.end();
        Value* dst = tuple_values.data();
        for (; pos != end; ++pos, ++dst) {
            new (dst) Value(*pos);
        }
    });
}

Value* Tuple::data() {
    return layout()->fixed_slots_begin();
}

size_t Tuple::size() {
    return layout()->fixed_slot_capacity();
}

Value Tuple::get(size_t index) {
    // TODO this should be a language level exception
    TIRO_CHECK(index < size(), "Tuple::get(): index out of bounds.");
    return *layout()->fixed_slot(index);
}

void Tuple::set(size_t index, Value value) {
    // TODO Exception
    TIRO_CHECK(index < size(), "Tuple::set(): index out of bounds.");
    *layout()->fixed_slot(index) = value;
}

template<typename Init>
Tuple Tuple::make_impl(Context& ctx, size_t size, Init&& init) {
    auto type = ctx.types().internal_type<Tuple>();
    size_t allocation_size = LayoutTraits<Layout>::dynamic_size(size);
    Layout* data = ctx.heap().create_varsize<Layout>(
        allocation_size, type, FixedSlotsInit(size, init));
    return Tuple(from_heap(data));
}

static constexpr MethodDesc tuple_methods[] = {
    {
        "size"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto tuple = check_instance<Tuple>(frame);
            frame.result(frame.ctx().get_integer(static_cast<i64>(tuple->size())));
        },
    },
};

constexpr TypeDesc tuple_type_desc{"Tuple"sv, tuple_methods};

} // namespace tiro::vm