#include "vm/objects/module.hpp"

#include "vm/objects/factory.hpp"
#include "vm/objects/hash_table.hpp"

namespace tiro::vm {

Module
Module::make(Context& ctx, Handle<String> name, Handle<Tuple> members, Handle<HashTable> exported) {
    Layout* data = create_object<Module>(ctx, StaticSlotsInit());
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

std::optional<Value> Module::find_exported(Handle<Symbol> name) {
    auto exp = exported();
    TIRO_DEBUG_ASSERT(exp, "Must have a table of exported members.");
    return exp.get(*name);
}

Value Module::init() {
    return layout()->read_static_slot(InitSlot);
}

void Module::init(MaybeHandle<Value> value) {
    layout()->write_static_slot(InitSlot, value.to_nullable());
}

} // namespace tiro::vm
