#include "vm/objects/record.hpp"

#include "vm/handles/scope.hpp"
#include "vm/object_support/factory.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/primitives.hpp"

namespace tiro::vm {

RecordSchema RecordSchema::make(Context& ctx, Handle<Array> keys) {
    Scope sc(ctx);
    Local props = sc.local(HashTable::make(ctx));
    Local key = sc.local();
    Local value = sc.local();
    for (size_t i = 0, n = keys->size(); i < n; ++i) {
        key = keys->unchecked_get(i);
        value = ctx.get_integer(i);

        TIRO_DEBUG_ASSERT(key->is<Symbol>(), "keys must be symbols");
        bool inserted = props->set(ctx, key, value).must("too many record keys");
        TIRO_CHECK(inserted, "record keys must be unique");
    }

    Layout* data = create_object<RecordSchema>(ctx, StaticSlotsInit());
    data->write_static_slot(PropertiesSlot, props);
    return RecordSchema(from_heap(data));
}

size_t RecordSchema::size() {
    return get_props().size();
}

std::optional<size_t> RecordSchema::index_of(Symbol symbol) {
    auto props = get_props();
    auto found_value = props.get(symbol);
    if (!found_value)
        return {};

    Value value = *found_value;
    TIRO_DEBUG_ASSERT(value.is<Integer>(), "value must be an integer");
    return value.must_cast<Integer>().try_extract_size();
}

HashTable RecordSchema::get_props() {
    return layout()->read_static_slot<HashTable>(PropertiesSlot);
}

Record Record::make(Context& ctx, Handle<Array> keys) {
    Scope sc(ctx);
    Local schema = sc.local(RecordSchema::make(ctx, keys));
    return make(ctx, schema);
}

Record Record::make(Context& ctx, Handle<RecordSchema> schema) {
    Scope sc(ctx);
    Local values = sc.local(Tuple::make(ctx, schema->size()));

    Layout* data = create_object<Record>(ctx, StaticSlotsInit());
    data->write_static_slot(SchemaSlot, schema);
    data->write_static_slot(ValuesSlot, values);
    return Record(from_heap(data));
}

Array Record::keys(Context& ctx, Handle<Record> record) {
    Scope sc(ctx);
    Local schema = sc.local(record->get_schema());
    Local keys = sc.local(Array::make(ctx, schema->size()));
    schema->for_each(ctx,
        [&](Handle<Symbol> symbol) { keys->append(ctx, symbol).must("failed to add record key"); });
    return *keys;
}

std::optional<Value> Record::get(Symbol key) {
    auto schema = get_schema();
    auto found_index = schema.index_of(key);
    if (!found_index)
        return {};

    size_t index = *found_index;
    auto values = get_values();
    TIRO_DEBUG_ASSERT(index < values.size(), "index too large");
    return values.checked_get(index);
}

bool Record::set(Symbol key, Value value) {
    auto schema = get_schema();
    auto found_index = schema.index_of(key);
    if (!found_index)
        return false;

    size_t index = *found_index;
    auto values = get_values();
    TIRO_DEBUG_ASSERT(index < values.size(), "index too large");
    values.unchecked_set(index, value);
    return true;
}

RecordSchema Record::get_schema() {
    return layout()->read_static_slot<RecordSchema>(SchemaSlot);
}

Tuple Record::get_values() {
    return layout()->read_static_slot<Tuple>(ValuesSlot);
}

} // namespace tiro::vm
