#include "vm/objects/class.hpp"

#include "vm/context.hpp"
#include "vm/handles/scope.hpp"
#include "vm/math.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/factory.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/string.hpp"
#include "vm/objects/type_desc.hpp"

namespace tiro::vm {

Method Method::make(Context& ctx, Handle<Value> function) {
    Layout* data = create_object<Method>(ctx, StaticSlotsInit());
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

    Layout* data = create_object<InternalType>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->static_payload()->builtin_type = builtin_type;
    return InternalType(from_heap(data));
}

ValueType InternalType::builtin_type() {
    return layout()->static_payload()->builtin_type;
}

Nullable<Type> InternalType::public_type() {
    return layout()->read_static_slot<Nullable<Type>>(PublicTypeSlot);
}

void InternalType::public_type(MaybeHandle<Type> type) {
    layout()->write_static_slot(PublicTypeSlot, type.to_nullable());
}

Type Type::make(Context& ctx, Handle<String> name, Handle<HashTable> members) {
    Layout* data = create_object<Type>(ctx, StaticSlotsInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(MembersSlot, members);
    return Type(from_heap(data));
}

String Type::name() {
    return layout()->read_static_slot<String>(NameSlot);
}

std::optional<Value> Type::find_member(Handle<Symbol> name) {
    return layout()->read_static_slot<HashTable>(MembersSlot).get(*name);
}

DynamicObject DynamicObject::make(Context& ctx) {
    Scope sc(ctx);
    Local props = sc.local(HashTable::make(ctx));

    Layout* data = create_object<DynamicObject>(ctx, StaticSlotsInit());
    data->write_static_slot(PropertiesSlot, props);
    return DynamicObject(from_heap(data));
}

Array DynamicObject::names(Context& ctx) {
    Scope sc(ctx);
    Local names = sc.local(Array::make(ctx, 0));
    Local props = sc.local(get_props());
    props->for_each(ctx, [&](auto key_handle, [[maybe_unused]] auto value_handle) {
        names->append(ctx, key_handle);
    });
    return *names;
}

Value DynamicObject::get(Handle<Symbol> name) {
    auto found = get_props().get(*name);
    return found ? *found : Value::null();
}

void DynamicObject::set(Context& ctx, Handle<Symbol> name, Handle<Value> value) {
    Scope sc(ctx);
    Local props = sc.local(get_props());
    if (value->is_null()) {
        props->remove(*name);
    } else {
        props->set(ctx, name, value);
    }
}

HashTable DynamicObject::get_props() {
    return layout()->read_static_slot<HashTable>(PropertiesSlot);
}

static const MethodDesc type_methods[] = {
    {
        "name"sv,
        1,
        NativeFunctionArg::sync([](NativeFunctionFrame& frame) {
            auto type = check_instance<Type>(frame);
            frame.result(type->name());
        }),
    },
};

const TypeDesc type_type_desc{"Type"sv, type_methods};

} // namespace tiro::vm
