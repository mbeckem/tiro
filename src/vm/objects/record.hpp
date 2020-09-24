#ifndef TIRO_VM_OBJECTS_RECORD_HPP
#define TIRO_VM_OBJECTS_RECORD_HPP

#include "vm/handles/handle.hpp"
#include "vm/objects/fwd.hpp"
#include "vm/objects/layout.hpp"
#include "vm/objects/value.hpp"

#include <optional>

namespace tiro::vm {

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

    explicit Record(Value v)
        : HeapValue(v, DebugCheck<Record>()) {}

    /// Returns the set of keys valid for this record.
    static Array keys(Context& ctx, Handle<Record> record);

    /// Returns the value associated with that key, or an empty optional if the key is invalid for this record.
    std::optional<Value> get(Symbol key);

    /// Sets the value associated with the given key. Returns true on success. Returns false (and does nothing)
    /// if the key is invalid for this record.
    static bool set(Context& ctx, Handle<Record> record, Handle<Symbol> key, Handle<Value> value);

    Layout* layout() const { return access_heap<Layout>(); }

private:
    static Record make_from_map(Context& ctx, Handle<HashTable> properties);

private:
    HashTable get_properties();
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_RECORD_HPP
