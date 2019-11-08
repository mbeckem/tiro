#ifndef HAMMER_VM_OBJECTS_ARRAY_STORAGE_BASE_IPP
#define HAMMER_VM_OBJECTS_ARRAY_STORAGE_BASE_IPP

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/array_storage_base.hpp"

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

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_ARRAY_STORAGE_BASE_IPP
