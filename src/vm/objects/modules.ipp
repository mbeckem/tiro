#ifndef TIRO_VM_OBJECTS_MODULES_IPP
#define TIRO_VM_OBJECTS_MODULES_IPP

#include "vm/objects/modules.hpp"

#include "vm/objects/hash_tables.hpp"
#include "vm/objects/strings.hpp"
#include "vm/objects/tuples.hpp"

namespace tiro::vm {

struct Module::Data : Header {
    Data(String name_, Tuple members_, HashTable exported_)
        : Header(ValueType::Module)
        , name(name_)
        , members(members_)
        , exported(exported_) {}

    String name;
    Tuple members;
    HashTable exported;
    Value init = Value::null();
};

size_t Module::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Module::walk(W&& w) {
    Data* d = access_heap();
    w(d->name);
    w(d->members);
    w(d->exported);
    w(d->init);
}

Module::Data* Module::access_heap() const {
    return Value::access_heap<Data>();
}

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_MODULES_IPP
