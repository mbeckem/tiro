#ifndef TIRO_VM_OBJECTS_ARRAY_HPP
#define TIRO_VM_OBJECTS_ARRAY_HPP

#include "common/defs.hpp"
#include "common/span.hpp"
#include "vm/heap/handles.hpp"
#include "vm/objects/array_storage_base.hpp"
#include "vm/objects/layout.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

/// Backing storage of an array. This is a contigous chunk of memory.
class ArrayStorage final : public ArrayStorageBase<Value, ArrayStorage> {
public:
    using ArrayStorageBase::ArrayStorageBase;
};

/// A dynamic, resizable array.
class Array final : public HeapValue {
public:
    using Layout = StaticLayout<StaticSlotsPiece<1>>;

    enum { StorageSlot = 0 };

    static Array make(Context& ctx, size_t initial_capacity);

    static Array make(Context& ctx,
        /* FIXME must be rooted */ Span<const Value> initial_content);

    Array() = default;

    explicit Array(Value v)
        : HeapValue(v, DebugCheck<Array>()) {}

    size_t size() const;     // Number of values in the array
    size_t capacity() const; // Total capacity before resize is needed
    const Value* data() const;
    Span<const Value> values() const { return {data(), size()}; }

    Value get(size_t index) const;
    void set(size_t index, Handle<Value> value);

    void append(Context& ctx, Handle<Value> value);
    void remove_last() const;

    Layout* layout() const { return access_heap<Layout>(); }

private:
    ArrayStorage get_storage() const {
        return layout()->read_static_slot<ArrayStorage>(StorageSlot);
    }

    void set_storage(ArrayStorage new_storage) {
        layout()->write_static_slot(StorageSlot, new_storage);
    }

    // Returns size >= required
    static size_t next_capacity(size_t required);
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_ARRAY_HPP
