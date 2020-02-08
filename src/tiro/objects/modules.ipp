#ifndef TIRO_OBJECTS_MODULES_IPP
#define TIRO_OBJECTS_MODULES_IPP

#include "tiro/objects/modules.hpp"

#include "tiro/objects/hash_tables.hpp"
#include "tiro/objects/strings.hpp"
#include "tiro/objects/tuples.hpp"

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

#endif // TIRO_OBJECTS_MODULES_IPP
