#ifndef TIRO_VM_OBJECTS_ARRAY_STORAGE_BASE_IPP
#define TIRO_VM_OBJECTS_ARRAY_STORAGE_BASE_IPP

#include "vm/objects/array_storage_base.hpp"

#include "vm/objects/array.hpp"
#include "vm/objects/factory.hpp"
#include "vm/objects/hash_table.hpp"

namespace tiro::vm {

template<typename T, typename Derived>
ArrayStorageBase<T, Derived>::ArrayStorageBase(Value v)
    : HeapValue(v, DebugCheck<Derived>()) {
    static_assert(std::is_trivially_destructible_v<T>,
        "Must be trivially destructible as destructors are not called.");
}

template<typename T, typename Derived>
Derived ArrayStorageBase<T, Derived>::make(Context& ctx, size_t capacity) {
    Layout* data = create_object<Derived>(ctx, capacity, DynamicSlotsInit(capacity));
    return Derived(from_heap(data));
}

template class ArrayStorageBase<Value, ArrayStorage>;
template class ArrayStorageBase<HashTableEntry, HashTableStorage>;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_ARRAY_STORAGE_BASE_IPP
