#include "vm/objects/tuple.hpp"

#include "vm/object_support/factory.hpp"
#include "vm/object_support/type_desc.hpp"

namespace tiro::vm {

Tuple Tuple::make(Context& ctx, size_t size) {
    return make_impl(ctx, size, [&](Span<Value> tuple_values) {
        TIRO_DEBUG_ASSERT(tuple_values.size() == size, "Unexpected tuple size.");
        std::uninitialized_fill_n(tuple_values.data(), tuple_values.size(), Value::null());
    });
}

Tuple Tuple::make(Context& ctx, HandleSpan<Value> initial_values) {
    return make_impl(ctx, initial_values.size(), [&](Span<Value> tuple_values) {
        TIRO_DEBUG_ASSERT(tuple_values.size() == initial_values.size(), "Unexpected tuple size.");
        auto raw_initial = initial_values.raw_slots();
        std::uninitialized_copy_n(raw_initial.data(), raw_initial.size(), tuple_values.data());
    });
}

Tuple Tuple::make(Context& ctx, HandleSpan<Value> initial_values, size_t size) {
    TIRO_DEBUG_ASSERT(
        size >= initial_values.size(), "Tuple::make(): invalid size, must be >= values.size().");

    const size_t copy = initial_values.size();
    const size_t fill = size - copy;
    return make_impl(ctx, size, [&](Span<Value> tuple_values) {
        TIRO_DEBUG_ASSERT(tuple_values.size() == size, "Unexpected tuple size.");
        auto raw_initial = initial_values.raw_slots();
        Value* pos = std::uninitialized_copy_n(raw_initial.data(), copy, tuple_values.data());
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
            new (dst) Value(**pos);
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
    Layout* data = create_object<Tuple>(ctx, size, FixedSlotsInit(size, init));
    return Tuple(from_heap(data));
}

TupleIterator TupleIterator::make(Context& ctx, Handle<Tuple> tuple) {
    Layout* data = create_object<TupleIterator>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(TupleSlot, tuple);
    return TupleIterator(from_heap(data));
}

std::optional<Value> TupleIterator::next() {
    Tuple tuple = layout()->read_static_slot<Tuple>(TupleSlot);
    size_t& index = layout()->static_payload()->index;
    if (index >= tuple.size())
        return {};

    return tuple.get(index++);
}

static void tuple_size_impl(NativeFunctionFrame& frame) {
    auto tuple = check_instance<Tuple>(frame);
    frame.return_value(frame.ctx().get_integer(static_cast<i64>(tuple->size())));
}

static const FunctionDesc tuple_methods[] = {
    FunctionDesc::method("size"sv, 1, NativeFunctionStorage::static_sync<tuple_size_impl>()),
};

const TypeDesc tuple_type_desc{"Tuple"sv, tuple_methods};

} // namespace tiro::vm
