#include "vm/objects/module.hpp"

#include "vm/object_support/factory.hpp"
#include "vm/objects/hash_table.hpp"

#include "vm/math.hpp"

namespace tiro::vm {

UnresolvedImport UnresolvedImport::make(Context& ctx, Handle<String> module_name) {
    Layout* data = create_object<UnresolvedImport>(ctx, StaticSlotsInit());
    data->write_static_slot(ModuleNameSlot, module_name);
    return UnresolvedImport(from_heap(data));
}

String UnresolvedImport::module_name() {
    return layout()->read_static_slot<String>(ModuleNameSlot);
}

Module
Module::make(Context& ctx, Handle<String> name, Handle<Tuple> members, Handle<HashTable> exported) {
    Layout* data = create_object<Module>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(MembersSlot, members);
    data->write_static_slot(ExportedSlot, exported);
    return Module(from_heap(data));
}

String Module::name() {
    return layout()->read_static_slot<String>(NameSlot);
}

Tuple Module::members() {
    return layout()->read_static_slot<Tuple>(MembersSlot);
}

HashTable Module::exported() {
    return layout()->read_static_slot<HashTable>(ExportedSlot);
}

std::optional<Value> Module::find_exported(Symbol name) {
    auto exported = this->exported();
    TIRO_DEBUG_ASSERT(exported, "Must have a table of exported members.");

    auto index = exported.get(name);
    if (!index)
        return {};

    auto index_value = Integer::try_extract(*index);
    TIRO_DEBUG_ASSERT(index_value, "Members of the exported table must always be integers.");

    auto members = this->members();
    TIRO_DEBUG_ASSERT(*index_value >= 0 && static_cast<size_t>(*index_value) < members.size(),
        "Index of exported module member is out of bounds.");
    return members.get(*index_value);
}

Value Module::initializer() {
    return layout()->read_static_slot(InitializerSlot);
}

void Module::initializer(Value value) {
    layout()->write_static_slot(InitializerSlot, value);
}

bool Module::initialized() {
    return layout()->static_payload()->initialized;
}

void Module::initialized(bool value) {
    layout()->static_payload()->initialized = value;
}

} // namespace tiro::vm
