#ifndef TIRO_VM_OBJECTS_ARRAYS_IPP
#define TIRO_VM_OBJECTS_ARRAYS_IPP

#include "vm/objects/arrays.hpp"

#include "vm/context.hpp"

namespace tiro::vm {

template<typename T, typename Derived>
template<typename Init>
Derived ArrayStorageBase<T, Derived>::make_impl(Context& ctx, size_t capacity, Init&& init) {
    const size_t allocation_size = variable_allocation<Data, T>(capacity);
    Data* data = ctx.heap().create_varsize<Data>(
        allocation_size, capacity, std::forward<Init>(init));
    TIRO_DEBUG_ASSERT(data->size <= data->capacity, "Size must be <= capacity.");
    return Derived(Value::from_heap(data));
}

struct Array::Data : Header {
    Data()
        : Header(ValueType::Array) {}

    ArrayStorage storage;
};

size_t Array::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Array::walk(W&& w) {
    Data* d = access_heap();
    w(d->storage);
}

Array::Data* Array::access_heap() const {
    return Value::access_heap<Data>();
}

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_ARRAYS_IPP
