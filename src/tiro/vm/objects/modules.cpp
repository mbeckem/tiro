#include "tiro/vm/objects/modules.hpp"

#include "tiro/vm/context.hpp"

#include "tiro/vm/objects/modules.ipp"

namespace tiro::vm {

Module Module::make(Context& ctx, Handle<String> name, Handle<Tuple> members,
    Handle<HashTable> exported) {
    Data* data = ctx.heap().create<Data>(name, members, exported);
    return Module(from_heap(data));
}

String Module::name() const {
    return access_heap<Data>()->name;
}

Tuple Module::members() const {
    return access_heap<Data>()->members;
}

HashTable Module::exported() const {
    return access_heap<Data>()->exported;
}

} // namespace tiro::vm
