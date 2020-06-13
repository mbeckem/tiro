#include "vm/objects/classes.hpp"

#include "vm/context.hpp"
#include "vm/math.hpp"
#include "vm/objects/arrays.hpp"
#include "vm/objects/hash_tables.hpp"
#include "vm/objects/strings.hpp"

#include "vm/objects/classes.ipp"

namespace tiro::vm {

Method Method::make(Context& ctx, Handle<Value> function) {
    Data* data = ctx.heap().create<Data>();
    data->function = function;
    return Method(from_heap(data));
}

Value Method::function() const {
    return access_heap()->function;
}

Symbol Symbol::make(Context& ctx, Handle<String> name) {
    TIRO_CHECK(!name->is_null(), "The symbol name must be a valid string.");

    Data* data = ctx.heap().create<Data>(name.get());
    return Symbol(from_heap(data));
}

String Symbol::name() const {
    return access_heap()->name;
}

bool Symbol::equal(Symbol other) const {
    return same(other);
}

DynamicObject DynamicObject::make(Context& ctx) {
    Root properties(ctx, HashTable::make(ctx));

    Data* data = ctx.heap().create<Data>();
    data->properties = properties;
    return DynamicObject(from_heap(data));
}

Array DynamicObject::properties(Context& ctx) const {
    Root names(ctx, Array::make(ctx, 0));
    Root props(ctx, access_heap()->properties);

    props->for_each(ctx, [&](auto key_handle, [[maybe_unused]] auto value_handle) {
        names->append(ctx, key_handle);
    });
    return names;
}

Value DynamicObject::get(Handle<Symbol> property) const {
    auto found = access_heap()->properties.get(property.get());
    return found ? *found : Value::null();
}

void DynamicObject::set(Context& ctx, Handle<Symbol> property, Handle<Value> value) {
    TIRO_DEBUG_ASSERT(property.get(), "Invalid property name.");

    Root props(ctx, access_heap()->properties);
    if (value->is_null()) {
        props->remove(property);
    } else {
        props->set(ctx, property, value);
    }
}

} // namespace tiro::vm
