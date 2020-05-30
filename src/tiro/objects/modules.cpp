#include "tiro/objects/modules.hpp"

#include "tiro/vm/context.hpp"

#include "tiro/objects/modules.ipp"

namespace tiro::vm {

Module
Module::make(Context& ctx, Handle<String> name, Handle<Tuple> members, Handle<HashTable> exported) {
    Data* data = ctx.heap().create<Data>(name, members, exported);
    return Module(from_heap(data));
}

String Module::name() const {
    return access_heap()->name;
}

Tuple Module::members() const {
    return access_heap()->members;
}

HashTable Module::exported() const {
    return access_heap()->exported;
}

Value Module::init() const {
    return access_heap()->init;
}

void Module::init(Handle<Value> value) const {
    access_heap()->init = value;
}

} // namespace tiro::vm
