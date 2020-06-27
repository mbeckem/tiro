#include "vm/objects/class.hpp"

#include "vm/context.hpp"
#include "vm/math.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/string.hpp"

namespace tiro::vm {

Method Method::make(Context& ctx, Handle<Value> function) {
    Layout* data = ctx.heap().create<Layout>(ValueType::Method, StaticSlotsInit());
    data->write_static_slot(FunctionSlot, function);
    return Method(from_heap(data));
}

Value Method::function() {
    return layout()->read_static_slot(FunctionSlot);
}

Type Type::make(Context& ctx, Handle<String> name, Handle<HashTable> methods) {
    Layout* data = ctx.heap().create<Layout>(ValueType::Type, StaticSlotsInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(MethodsSlot, methods);
    return Type(from_heap(data));
}

String Type::name() {
    return layout()->read_static_slot<String>(NameSlot);
}

std::optional<Method> Type::find_method(Handle<Symbol> name) {
    auto methods = get_methods();
    if (auto found = methods.get(name))
        return found->as<Method>();
    return {};
}

HashTable Type::get_methods() {
    return layout()->read_static_slot<HashTable>(MethodsSlot);
}

DynamicObject DynamicObject::make(Context& ctx) {
    Root props(ctx, HashTable::make(ctx));

    Layout* data = ctx.heap().create<Layout>(ValueType::DynamicObject, StaticSlotsInit());
    data->write_static_slot(PropertiesSlot, props.get());
    return DynamicObject(from_heap(data));
}

Array DynamicObject::names(Context& ctx) {
    Root names(ctx, Array::make(ctx, 0));
    Root props(ctx, get_props());

    props->for_each(ctx, [&](auto key_handle, [[maybe_unused]] auto value_handle) {
        names->append(ctx, key_handle);
    });
    return names;
}

Value DynamicObject::get(Handle<Symbol> name) {
    auto found = get_props().get(name.get());
    return found ? *found : Value::null();
}

void DynamicObject::set(Context& ctx, Handle<Symbol> name, Handle<Value> value) {
    TIRO_DEBUG_ASSERT(name.get(), "Invalid property name.");

    Root props(ctx, get_props());
    if (value->is_null()) {
        props->remove(name);
    } else {
        props->set(ctx, name, value);
    }
}

HashTable DynamicObject::get_props() {
    return layout()->read_static_slot<HashTable>(PropertiesSlot);
}

} // namespace tiro::vm
