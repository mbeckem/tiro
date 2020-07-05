#ifndef TIRO_VM_OBJECTS_ARRAY_STORAGE_BASE_IPP
#define TIRO_VM_OBJECTS_ARRAY_STORAGE_BASE_IPP

#include "vm/objects/array_storage_base.hpp"

#include "vm/objects/factory.hpp"

namespace tiro::vm {

template<typename T, typename Derived>
Derived ArrayStorageBase<T, Derived>::make(Context& ctx, size_t capacity) {
    Layout* data = create_object<Derived>(ctx, capacity, DynamicSlotsInit(capacity));
    return Derived(from_heap(data));
}

template<typename T, typename Derived>
Derived
ArrayStorageBase<T, Derived>::make(Context& ctx, Span<const T> initial_content, size_t capacity) {
    TIRO_DEBUG_ASSERT(initial_content.size() <= capacity,
        "ArrayStorageBase::make(): initial content does not fit into the "
        "capacity.");

    Layout* data = create_object<Derived>(ctx, capacity, DynamicSlotsInit(capacity));
    data->add_dynamic_slots(initial_content);
    return Derived(from_heap(data));
}

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_ARRAY_STORAGE_BASE_IPP
