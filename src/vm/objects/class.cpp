#include "vm/objects/class.hpp"

#include "vm/context.hpp"
#include "vm/handles/scope.hpp"
#include "vm/math.hpp"
#include "vm/object_support/factory.hpp"
#include "vm/object_support/type_desc.hpp"
#include "vm/objects/array.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/string.hpp"

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
    Layout* data = detail::create_impl<Layout>(
        ctx.heap(), nullptr, StaticSlotsInit(), StaticPayloadInit());
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

static void class_name_impl(NativeFunctionFrame& frame) {
    auto type = check_instance<Type>(frame);
    frame.return_value(type->name());
}

static constexpr FunctionDesc type_methods[] = {
    FunctionDesc::method("name"sv, 1, NativeFunctionStorage::static_sync<class_name_impl>()),
};

constexpr TypeDesc type_type_desc{"Type"sv, type_methods};

} // namespace tiro::vm
