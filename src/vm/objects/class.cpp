#include "vm/objects/class.hpp"

#include "vm/context.hpp"
#include "vm/math.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/string.hpp"

namespace tiro::vm {

Method Method::make(Context& ctx, Handle<Value> function) {
    auto type = ctx.types().internal_type<Method>();
    Layout* data = ctx.heap().create<Layout>(type, StaticSlotsInit());
    data->write_static_slot(FunctionSlot, function);
    return Method(from_heap(data));
}

Value Method::function() {
    return layout()->read_static_slot(FunctionSlot);
}

InternalType InternalType::make_root(Context& ctx) {
    // Root type is its own type.
    Layout* data = ctx.heap().create<Layout>(nullptr, StaticSlotsInit(), StaticPayloadInit());
    data->type(data);
    data->static_payload()->builtin_type = ValueType::InternalType;
    return InternalType(from_heap(data));
}

InternalType InternalType::make(Context& ctx, ValueType builtin_type) {
    TIRO_DEBUG_ASSERT(
        builtin_type != ValueType::InternalType, "Use make_root() to represent the root type.");

    auto type = ctx.types().internal_type<InternalType>();
    Layout* data = ctx.heap().create<Layout>(type, StaticSlotsInit(), StaticPayloadInit());
    data->static_payload()->builtin_type = builtin_type;
    return InternalType(from_heap(data));
}

ValueType InternalType::builtin_type() {
    return layout()->static_payload()->builtin_type;
}

Type InternalType::public_type() {
    return layout()->read_static_slot<Type>(PublicTypeSlot);
}

void InternalType::public_type(Handle<Type> type) {
    layout()->write_static_slot(PublicTypeSlot, type);
}

Type Type::make(Context& ctx, Handle<String> name, Handle<HashTable> methods) {
    auto type = ctx.types().internal_type<Type>();
    Layout* data = ctx.heap().create<Layout>(type, StaticSlotsInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(MethodsSlot, methods);
    return Type(from_heap(data));
}

String Type::name() {
    return layout()->read_static_slot<String>(NameSlot);
}

std::optional<Method> Type::find_method(Handle<Symbol> name) {
    auto methods = layout()->read_static_slot<HashTable>(MethodsSlot);
    if (auto found = methods.get(name))
        return found->as<Method>();
    return {};
}

DynamicObject DynamicObject::make(Context& ctx) {
    Root props(ctx, HashTable::make(ctx));

    auto type = ctx.types().internal_type<DynamicObject>();
    Layout* data = ctx.heap().create<Layout>(type, StaticSlotsInit());
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

static constexpr MethodDesc type_methods[] = {
    {
        "name"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto type = check_instance<Type>(frame);
            frame.result(type->name());
        },
    },
};

constexpr TypeDesc type_type_desc{"Type"sv, type_methods};

} // namespace tiro::vm
