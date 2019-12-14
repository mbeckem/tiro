#ifndef HAMMER_VM_OBJECTS_MODULES_IPP
#define HAMMER_VM_OBJECTS_MODULES_IPP

#include "hammer/vm/objects/modules.hpp"

#include "hammer/vm/objects/hash_tables.hpp"
#include "hammer/vm/objects/strings.hpp"
#include "hammer/vm/objects/tuples.hpp"

namespace hammer::vm {

struct Module::Data : Header {
    Data(String name_, Tuple members_, HashTable exported_)
        : Header(ValueType::Module)
        , name(name_)
        , members(members_)
        , exported(exported_) {}

    String name;
    Tuple members;
    HashTable exported;
};

size_t Module::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Module::walk(W&& w) {
    Data* d = access_heap<Data>();
    w(d->name);
    w(d->members);
    w(d->exported);
}

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_MODULES_IPP
