#include "vm/objects/tuples.hpp"

#include "vm/context.hpp"

#include "vm/objects/tuples.ipp"

namespace tiro::vm {

Tuple Tuple::make(Context& ctx, size_t size) {
    return make_impl(
        ctx, size, [&](Data* d) { std::uninitialized_fill_n(d->values, size, Value::null()); });
}

Tuple Tuple::make(Context& ctx, Span<const Value> values) {
    return make_impl(ctx, values.size(),
        [&](Data* d) { std::uninitialized_copy_n(values.data(), values.size(), d->values); });
}

Tuple Tuple::make(Context& ctx, Span<const Value> values, size_t total_values) {
    TIRO_DEBUG_ASSERT(total_values >= values.size(),
        "Tuple::make(): invalid total_size, must be >= values.size().");

    const size_t copy = values.size();
    const size_t fill = total_values - copy;
    return make_impl(ctx, total_values, [&](Data* d) {
        Value* pos = std::uninitialized_copy_n(values.data(), copy, d->values);
        std::uninitialized_fill_n(pos, fill, Value::null());
    });
}

Tuple Tuple::make(Context& ctx, std::initializer_list<Handle<Value>> values) {
    return make_impl(ctx, values.size(), [&](Data* d) {
        const Handle<Value>* pos = values.begin();
        const Handle<Value>* end = values.end();
        Value* dst = d->values;
        for (; pos != end; ++pos, ++dst) {
            new (dst) Value(*pos);
        }
    });
}

const Value* Tuple::data() const {
    return access_heap()->values;
}

size_t Tuple::size() const {
    return access_heap()->size;
}

Value Tuple::get(size_t index) const {
    // TODO this should be a language level exception
    TIRO_CHECK(index < size(), "Tuple::get(): index out of bounds.");
    return access_heap()->values[index];
}

void Tuple::set(size_t index, Value value) const {
    // TODO Exception
    TIRO_CHECK(index < size(), "Tuple::set(): index out of bounds.");
    access_heap()->values[index] = value;
}

template<typename Init>
Tuple Tuple::make_impl(Context& ctx, size_t total_size, Init&& init) {
    const size_t allocation_size = variable_allocation<Data, Value>(total_size);
    Data* data = ctx.heap().create_varsize<Data>(
        allocation_size, total_size, std::forward<Init>(init));
    return Tuple(from_heap(data));
}

} // namespace tiro::vm
