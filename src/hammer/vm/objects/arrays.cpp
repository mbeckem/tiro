#include "hammer/vm/objects/arrays.hpp"

#include "hammer/vm/context.ipp"
#include "hammer/vm/objects/arrays.ipp"

#include <new>

namespace hammer::vm {

Array Array::make(Context& ctx, size_t initial_capacity) {
    Root<ArrayStorage> storage(ctx);
    if (initial_capacity > 0) {
        storage.set(ArrayStorage::make(ctx, initial_capacity));
    }

    Data* data = ctx.heap().create<Data>();
    data->storage = storage;
    return Array(from_heap(data));
}

Array Array::make(Context& ctx, Span<const Value> initial_content) {
    if (initial_content.empty())
        return make(ctx, 0);

    Root<ArrayStorage> storage(
        ctx, ArrayStorage::make(ctx, initial_content, initial_content.size()));

    Data* data = ctx.heap().create<Data>();
    data->storage = storage;
    return Array(from_heap(data));
}

size_t Array::size() const {
    Data* d = access_heap();
    return d->storage ? d->storage.size() : 0;
}

size_t Array::capacity() const {
    Data* d = access_heap();
    return d->storage ? d->storage.capacity() : 0;
}

const Value* Array::data() const {
    Data* d = access_heap();
    return d->storage.is_null() ? nullptr : d->storage.data();
}

Value Array::get(size_t index) const {
    // TODO Exception
    HAMMER_CHECK(index < size(), "Array::get(): index out of bounds.");
    return access_heap()->storage.get(index);
}

void Array::set(size_t index, Handle<Value> value) const {
    // TODO Exception
    HAMMER_CHECK(index < size(), "Array::set(): index out of bounds.");
    Data* d = access_heap();
    d->storage.set(index, value.get());
}

void Array::append(Context& ctx, Handle<Value> value) const {
    Data* const d = access_heap();

    size_t cap = capacity();
    if (size() >= cap) {
        if (HAMMER_UNLIKELY(!checked_add(cap, size_t(1)))) {
            HAMMER_ERROR("Array size too large."); // FIXME exception
        }
        cap = next_capacity(cap);

        Root<ArrayStorage> new_storage(ctx);
        if (d->storage) {
            new_storage.set(ArrayStorage::make(ctx, d->storage.values(), cap));
        } else {
            new_storage.set(ArrayStorage::make(ctx, cap));
        }

        // TODO: This would need a write barrier (assignment to d->storage).
        d->storage = new_storage;
    }

    HAMMER_ASSERT(size() < capacity(), "There must be enough free capacity.");
    d->storage.append(value);
}

void Array::remove_last() const {
    HAMMER_CHECK(size() > 0, "Array::remove_last(): Array is empty.");

    Data* d = access_heap();
    HAMMER_ASSERT(d->storage, "Invalid storage reference.");
    d->storage.remove_last();
}

size_t Array::next_capacity(size_t required) {
    static constexpr size_t max_pow = max_pow2<size_t>();
    if (HAMMER_UNLIKELY(required > max_pow)) {
        return std::numeric_limits<size_t>::max();
    }

    if (required == 0)
        return 0;
    if (required <= 8)
        return 8;
    return ceil_pow2(required);
}

} // namespace hammer::vm
