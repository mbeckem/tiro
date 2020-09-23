#include "vm/objects/record.hpp"

#include "vm/handles/scope.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/factory.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/primitives.hpp"

namespace tiro::vm {

Record Record::make(Context& ctx, Handle<Array> keys) {
    Scope sc(ctx);
    Local properties = sc.local(HashTable::make(ctx));
    Local key = sc.local();
    for (size_t i = 0, n = keys->size(); i < n; ++i) {
        key = keys->get(i);
        TIRO_CHECK(key->is<Symbol>(), "All keys of a record must be symbols.");
        bool inserted = properties->set(ctx, key, null_handle());
        TIRO_CHECK(inserted, "Keys must be unique.");
    }
    return make_from_map(ctx, properties);
}

Record Record::make(Context& ctx, HandleSpan<Symbol> symbols) {
    Scope sc(ctx);
    Local properties = sc.local(HashTable::make(ctx));
    for (auto symbol : symbols) {
        bool inserted = properties->set(ctx, symbol, null_handle());
        TIRO_CHECK(inserted, "Keys must be unique.");
    }
    return make_from_map(ctx, properties);
}

Array Record::keys(Context& ctx, Handle<Record> record) {
    Scope sc(ctx);
    Local properties = sc.local(record->get_properties());
    Local keys = sc.local(Array::make(ctx, properties->size()));
    properties->for_each(ctx, [&](Handle<Value> key, Handle<Value> value) {
        TIRO_DEBUG_ASSERT(key->is<Symbol>(), "All keys must be symbols.");
        keys->append(ctx, key);
        (void) value;
    });
    return *keys;
}

std::optional<Value> Record::get(Symbol key) {
    HashTable table = get_properties();
    return table.get(key);
}

bool Record::set(Context& ctx, Handle<Record> record, Handle<Symbol> key, Handle<Value> value) {
    // Note: scope is not really necessary here from an optimization standpoint because
    // no property can be added, so no allocation can occurr (the set of keys is fixed at construction).
    Scope sc(ctx);
    Local properties = sc.local(record->get_properties());

    // Note: there is no find API yet, so we do a contains() check first.
    if (!properties->contains(*key))
        return false;

    properties->set(ctx, key, value);
    return true;
}

HashTable Record::get_properties() {
    return layout()->read_static_slot<HashTable>(PropertiesSlot);
}

Record Record::make_from_map(Context& ctx, Handle<HashTable> properties) {
    Layout* data = create_object<Record>(ctx, StaticSlotsInit());
    data->write_static_slot(PropertiesSlot, properties);
    return Record(from_heap(data));
}

} // namespace tiro::vm
