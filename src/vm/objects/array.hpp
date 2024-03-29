#ifndef TIRO_VM_OBJECTS_ARRAY_HPP
#define TIRO_VM_OBJECTS_ARRAY_HPP

#include "common/adt/span.hpp"
#include "common/defs.hpp"
#include "vm/handles/handle.hpp"
#include "vm/handles/span.hpp"
#include "vm/object_support/fwd.hpp"
#include "vm/object_support/layout.hpp"
#include "vm/objects/array_storage_base.hpp"
#include "vm/objects/nullable.hpp"
#include "vm/objects/value.hpp"

#include <optional>

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

    static Array make(Context& ctx);
    static Array make(Context& ctx, size_t initial_capacity);
    static Array make(Context& ctx, HandleSpan<Value> initial_content);

    explicit Array(Value v)
        : HeapValue(v, DebugCheck<Array>()) {}

    size_t size();     // Number of values in the array
    size_t capacity(); // Total capacity before resize is needed
    Value* data();
    Span<Value> values() { return {data(), size()}; }

    /// Returns the item at the given index.
    /// Item access is unchecked.
    ///
    /// \pre `index < size()`.
    Value unchecked_get(size_t index);

    /// Sets the item at the given index.
    /// Item access is unchecked.
    ///
    /// \pre `index < size()`.
    void unchecked_set(size_t index, Value value);

    /// Returns the item at the given index.
    /// The item index is checked at runtime, and an internal error
    /// is thrown when the index is out of bounds.
    Value checked_get(size_t index);

    /// Sets the item at the given index.
    /// The item index is checked at runtime, and an internal error
    /// is thrown when the index is out of bounds.
    void checked_set(size_t index, Value value);

    bool try_append(Context& ctx, Handle<Value> value);
    Fallible<void> append(Context& ctx, Handle<Value> value);

    void remove_last();

    void clear();

    Layout* layout() const { return access_heap<Layout>(); }

private:
    Nullable<ArrayStorage> get_storage() {
        return layout()->read_static_slot<Nullable<ArrayStorage>>(StorageSlot);
    }

    void set_storage(Nullable<ArrayStorage> new_storage) {
        layout()->write_static_slot(StorageSlot, new_storage);
    }

    // Returns size >= required
    static size_t next_capacity(size_t required);
};

/// Iterates over an array.
class ArrayIterator final : public HeapValue {
private:
    enum Slots {
        ArraySlot,
        SlotCount_,
    };

    struct Payload {
        size_t index = 0;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    static ArrayIterator make(Context& ctx, Handle<Array> array);

    explicit ArrayIterator(Value v)
        : HeapValue(v, DebugCheck<ArrayIterator>()) {}

    std::optional<Value> next();

    Layout* layout() const { return access_heap<Layout>(); }
};

extern const TypeDesc array_type_desc;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_ARRAY_HPP
