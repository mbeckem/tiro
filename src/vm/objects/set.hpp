#ifndef TIRO_VM_OBJECTS_SET_HPP
#define TIRO_VM_OBJECTS_SET_HPP

#include "common/defs.hpp"
#include "vm/handles/handle.hpp"
#include "vm/object_support/fwd.hpp"
#include "vm/object_support/layout.hpp"
#include "vm/objects/fwd.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/value.hpp"

#include <optional>

namespace tiro::vm {

class Set final : public HeapValue {
private:
    enum Slots {
        TableSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    /// Creates a new, empty set.
    static Set make(Context& ctx);

    /// Creates a new, empty set with enough room for `initial_capacity` elements.
    static Fallible<Set> make(Context& ctx, size_t initial_capacity);

    /// Creates a new set with the given initial content.
    static Fallible<Set> make(Context& ctx, HandleSpan<Value> initial_content);

    explicit Set(Value v)
        : HeapValue(v, DebugCheck<Set>()) {}

    /// Returns true if a value equal to `v` exists in the set.
    bool contains(Value v);

    /// Returns the value in this set that is equal to `v`, if it exists.
    std::optional<Value> find(Value v);

    /// Returns the number of elements in this set.
    size_t size();

    /// Attempts to insert the given value into the set.
    /// Returns true if the value was successfully inserted.
    /// Returns false (and does nothing) if a value equal to `v` already exists.
    Fallible<bool> insert(Context& ctx, Handle<Value> v);

    /// Removes the value equal to `v` from this set, if it exists.
    /// TODO: Old value?
    void remove(Value v);

    /// Removes all elements from this set.
    void clear();

    /// Unsafe iteration over the set's items. No gc allocation can be made.
    template<typename Function>
    void for_each_unsafe(Function&& fn) {
        HashTable table = get_table();
        table.for_each_unsafe([&](Value key, Value value) {
            (void) value;
            fn(key);
        });
    }

    Layout* layout() const { return access_heap<Layout>(); }

private:
    friend SetIterator;

    HashTable get_table();
};

/// Iterates over the values in a set.
class SetIterator final : public HeapValue {
private:
    enum Slots {
        IterSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    static SetIterator make(Context& ctx, Handle<Set> set);

    explicit SetIterator(Value v)
        : HeapValue(v, DebugCheck<SetIterator>()) {}

    std::optional<Value> next(Context& ctx);

    Layout* layout() const { return access_heap<Layout>(); }
};

extern const TypeDesc set_type_desc;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_SET_HPP
