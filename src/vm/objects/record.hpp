#ifndef TIRO_VM_OBJECTS_RECORD_HPP
#define TIRO_VM_OBJECTS_RECORD_HPP

#include "vm/handles/handle.hpp"
#include "vm/handles/scope.hpp"
#include "vm/object_support/layout.hpp"
#include "vm/objects/fwd.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/primitives.hpp"
#include "vm/objects/value.hpp"

#include <optional>

namespace tiro::vm {

/// A record schema contains the keys for the construction of record instances.
///
/// TODO: This initial implementation is not very efficient (records have their own hash tables).
/// Records should simply be a dynamic array of flat slots (only containing values) with a pointer
/// to the immutable template for name -> value index mapping.
/// This should be implemented when classes exist, since they need a similar machinery.
class RecordSchema final : public HeapValue {
private:
    enum { PropertiesSlot, SlotCount_ };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    /// Creates a new record schema with the given property keys. All keys must be (unique) symbols.
    static RecordSchema make(Context& ctx, Handle<Array> keys);

    explicit RecordSchema(Value v)
        : HeapValue(v, DebugCheck<RecordSchema>()) {}

    /// Returns the number of properties configured for this template.
    size_t size();

    /// Iterates over all symbols in the record schema.
    template<typename Iter>
    void for_each(Context& ctx, Iter&& iter) {
        Scope sc(ctx);
        Local props = sc.local(get_props());
        props->template for_each(
            ctx, [&](auto key_handle, auto) { iter(key_handle.template must_cast<Symbol>()); });
    }

    Layout* layout() const { return access_heap<Layout>(); }

private:
    HashTable get_props();
};

/// A record is a simple key-value mapping datastructure. Arbitrary keys (of type symbol) can be
/// specified during construction, which can then be associated with arbitrary values of any type.
/// The set of keys cannot be altered after a record has been constructed.
///
/// TODO: Records should have a schema (an instance of InternalType?) that specifies their properties.
/// This would deduplicate many of the string keys and make construction efficient.
/// They should share the same infrastructure (key to slot mapping and so on) as classes.
class Record final : public HeapValue {
private:
    enum {
        PropertiesSlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    /// Creates a new record with the given property keys. All keys must be symbols.
    /// The values associated with these keys will be initialized to null.
    static Record make(Context& ctx, Handle<Array> keys);

    /// Creates a new record with the given property keys.
    /// The values associated with these keys will be initialized to null.
    static Record make(Context& ctx, HandleSpan<Symbol> symbols);

    /// Creates a new record from an existing template. All values are initialized to null.
    static Record make(Context& ctx, Handle<RecordSchema> tmpl);

    explicit Record(Value v)
        : HeapValue(v, DebugCheck<Record>()) {}

    /// Returns the set of keys valid for this record.
    /// TODO: This data should live in the record schema and should be immutable.
    /// This function should just return an iterable to tiro code.
    static Array keys(Context& ctx, Handle<Record> record);

    /// Returns the value associated with that key, or an empty optional if the key is invalid for this record.
    std::optional<Value> get(Symbol key);

    /// Sets the value associated with the given key. Returns true on success. Returns false (and does nothing)
    /// if the key is invalid for this record.
    static bool set(Context& ctx, Handle<Record> record, Handle<Symbol> key, Handle<Value> value);

    /// Quick-and-dirty iteration for record inspection without allocation.
    /// TODO: Should be replaced with a link to the record schema, which should store the keys.
    template<typename Function>
    void for_each_unsafe(Function&& fn) {
        HashTable props = get_props();
        props.for_each_unsafe([&](Value k, Value v) {
            TIRO_DEBUG_ASSERT(k.is<Symbol>(), "Record keys must always be symbols.");
            fn(k.template must_cast<Symbol>(), v);
        });
    }

    Layout* layout() const { return access_heap<Layout>(); }

private:
    static Record make_from_map(Context& ctx, Handle<HashTable> properties);

private:
    HashTable get_props();
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_RECORD_HPP
