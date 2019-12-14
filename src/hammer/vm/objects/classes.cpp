#include "hammer/vm/objects/classes.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/math.hpp"
#include "hammer/vm/objects/array.hpp"
#include "hammer/vm/objects/hash_table.hpp"
#include "hammer/vm/objects/string.hpp"

#include "hammer/vm/objects/classes.ipp"

namespace hammer::vm {

Method Method::make(Context& ctx, Handle<Value> function) {
    Data* data = ctx.heap().create<Data>();
    data->function = function;
    return Method(from_heap(data));
}

Value Method::function() const {
    return access_heap()->function;
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

    props->for_each(
        ctx, [&](auto key_handle, [[maybe_unused]] auto value_handle) {
            names->append(ctx, key_handle);
        });
    return names;
}

Value DynamicObject::get(Handle<Symbol> property) const {
    auto found = access_heap()->properties.get(property.get());
    return found ? *found : Value::null();
}

void DynamicObject::set(
    Context& ctx, Handle<Symbol> property, Handle<Value> value) {
    HAMMER_ASSERT(property.get(), "Invalid property name.");

    Root props(ctx, access_heap()->properties);
    if (value->is_null()) {
        props->remove(property);
    } else {
        props->set(ctx, property, value);
    }
}

} // namespace hammer::vm
