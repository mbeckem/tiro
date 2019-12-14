#ifndef HAMMER_VM_OBJECTS_ARRAYS_IPP
#define HAMMER_VM_OBJECTS_ARRAYS_IPP

#include "hammer/vm/objects/arrays.hpp"

#include "hammer/vm/context.hpp"

namespace hammer::vm {

template<typename T, typename Derived>
template<typename Init>
Derived ArrayStorageBase<T, Derived>::make_impl(
    Context& ctx, size_t capacity, Init&& init) {
    const size_t allocation_size = variable_allocation<Data, T>(capacity);
    Data* data = ctx.heap().create_varsize<Data>(
        allocation_size, capacity, std::forward<Init>(init));
    HAMMER_ASSERT(data->size <= data->capacity, "Size must be <= capacity.");
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

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_ARRAYS_IPP
