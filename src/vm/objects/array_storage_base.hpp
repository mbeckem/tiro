#ifndef TIRO_VM_OBJECTS_ARRAY_STORAGE_BASE_HPP
#define TIRO_VM_OBJECTS_ARRAY_STORAGE_BASE_HPP

#include "common/adt/span.hpp"
#include "common/defs.hpp"
#include "vm/handles/handle.hpp"
#include "vm/handles/span.hpp"
#include "vm/object_support/layout.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

/// Provides the underyling storage for array objects that can contain references to
/// other objects. ArrayStorage objects are contiguous in memory.
/// They consist of an occupied part (from index 0 to size()) and an uninitialized part (from size() to capacity()).
///
/// This has the advantage that the garbage collector only has to scan the occupied part, as the uninitialized part
/// is guaranteed not to contain any valid references.
template<typename T, typename Derived>
class ArrayStorageBase : public HeapValue {
public:
    using Layout = DynamicSlotsLayout<T>;

    static Derived make(Context& ctx, size_t capacity);

    explicit ArrayStorageBase(Value v);

    size_t size() { return layout()->dynamic_slot_count(); }
    size_t capacity() { return layout()->dynamic_slot_capacity(); }
    T* data() { return layout()->dynamic_slots_begin(); }
    Span<T> values() { return layout()->dynamic_slots(); }

    bool empty() {
        Layout* data = layout();
        return data->dynamic_slot_count() == 0;
    }

    bool full() {
        Layout* data = layout();
        return data->dynamic_slot_count() == data->dynamic_slot_capacity();
    }

    T& get(size_t index) {
        TIRO_DEBUG_ASSERT(index < size(), "ArrayStorageBase::get(): index out of bounds.");
        return *layout()->dynamic_slot(index);
    }

    void set(size_t index, T value) {
        TIRO_DEBUG_ASSERT(index < size(), "ArrayStorageBase::set(): index out of bounds.");
        *layout()->dynamic_slot(index) = value;
    }

    void append(const T& value) {
        TIRO_DEBUG_ASSERT(
            size() < capacity(), "ArrayStorageBase::append(): no free capacity remaining.");

        Layout* data = layout();
        data->add_dynamic_slot(value);
    }

    void append_all(Span<const T> values) {
        TIRO_DEBUG_ASSERT(values.size() <= capacity() - size(),
            "ArrayStorageBase::append_all(): not enough capacity remaining.");

        Layout* data = layout();
        data->add_dynamic_slots(values);
    }

    void clear() { layout()->clear_dynamic_slots(); }

    void remove_last() {
        TIRO_DEBUG_ASSERT(size() > 0, "ArrayStorageBase::remove_last(): storage is empty.");
        layout()->remove_dynamic_slot();
    }

    void remove_last(size_t n) {
        TIRO_DEBUG_ASSERT(n <= size(),
            "ArrayStorageBase::remove_last(): cannot remove that many "
            "elements.");
        layout()->remove_dynamic_slots(n);
    }

    Layout* layout() const { return HeapValue::access_heap<Layout>(); }
};

extern template class ArrayStorageBase<Value, ArrayStorage>;
extern template class ArrayStorageBase<HashTableEntry, HashTableStorage>;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_ARRAY_STORAGE_BASE_HPP
