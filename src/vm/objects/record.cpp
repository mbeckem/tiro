#include "vm/objects/record.hpp"

#include "vm/handles/scope.hpp"
#include "vm/object_support/factory.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/primitives.hpp"

namespace tiro::vm {

RecordTemplate RecordTemplate::make(Context& ctx, Handle<Array> keys) {
    Scope sc(ctx);
    Local props = sc.local(HashTable::make(ctx));
    Local key = sc.local();
    Local value = sc.local();
    for (size_t i = 0, n = keys->size(); i < n; ++i) {
        key = keys->get(i);
        value = ctx.get_integer(i);

        TIRO_CHECK(key->is<Symbol>(), "All keys of a record must be symbols.");
        bool inserted = props->set(ctx, key, value);
        TIRO_CHECK(inserted, "Keys must be unique.");
    }

    Layout* data = create_object<RecordTemplate>(ctx, StaticSlotsInit());
    data->write_static_slot(PropertiesSlot, props);
    return RecordTemplate(from_heap(data));
}

size_t RecordTemplate::size() {
    return get_props().size();
}

HashTable RecordTemplate::get_props() {
    return layout()->read_static_slot<HashTable>(PropertiesSlot);
}

Record Record::make(Context& ctx, Handle<Array> keys) {
    Scope sc(ctx);
    Local props = sc.local(HashTable::make(ctx));
    Local key = sc.local();
    for (size_t i = 0, n = keys->size(); i < n; ++i) {
        key = keys->get(i);
        TIRO_CHECK(key->is<Symbol>(), "All keys of a record must be symbols.");
        bool inserted = props->set(ctx, key, null_handle());
        TIRO_CHECK(inserted, "Keys must be unique.");
    }
    return make_from_map(ctx, props);
}

Record Record::make(Context& ctx, HandleSpan<Symbol> symbols) {
    Scope sc(ctx);
    Local props = sc.local(HashTable::make(ctx));
    for (auto symbol : symbols) {
        bool inserted = props->set(ctx, symbol, null_handle());
        TIRO_CHECK(inserted, "Keys must be unique.");
    }
    return make_from_map(ctx, props);
}

Record Record::make(Context& ctx, Handle<RecordTemplate> tmpl) {
    Scope sc(ctx);
    Local props = sc.local(HashTable::make(ctx, tmpl->size()));
    tmpl->for_each(ctx, [&](auto symbol) {
        [[maybe_unused]] bool inserted = props->set(ctx, symbol, null_handle());
        TIRO_DEBUG_ASSERT(inserted, "Symbol key was not unique.");
    });
    return make_from_map(ctx, props);
}

Array Record::keys(Context& ctx, Handle<Record> record) {
    Scope sc(ctx);
    Local props = sc.local(record->get_props());
    Local keys = sc.local(Array::make(ctx, props->size()));
    props->for_each(ctx, [&](Handle<Value> key, Handle<Value> value) {
        TIRO_DEBUG_ASSERT(key->is<Symbol>(), "All keys must be symbols.");
        keys->append(ctx, key).must("failed to add record key");
        (void) value;
    });
    return *keys;
}

std::optional<Value> Record::get(Symbol key) {
    HashTable table = get_props();
    return table.get(key);
}

bool Record::set(Context& ctx, Handle<Record> record, Handle<Symbol> key, Handle<Value> value) {
    // Note: scope is not really necessary here from an optimization standpoint because
    // no property can be added, so no allocation can occurr (the set of keys is fixed at construction).
    Scope sc(ctx);
    Local props = sc.local(record->get_props());

    // Note: there is no find API yet, so we do a contains() check first.
    if (!props->contains(*key))
        return false;

    props->set(ctx, key, value);
    return true;
}

HashTable Record::get_props() {
    return layout()->read_static_slot<HashTable>(PropertiesSlot);
}

Record Record::make_from_map(Context& ctx, Handle<HashTable> props) {
    Layout* data = create_object<Record>(ctx, StaticSlotsInit());
    data->write_static_slot(PropertiesSlot, props);
    return Record(from_heap(data));
}

} // namespace tiro::vm
