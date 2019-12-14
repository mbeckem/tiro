#ifndef HAMMER_VM_OBJECTS_CLASSES_IPP
#define HAMMER_VM_OBJECTS_CLASSES_IPP

#include "hammer/vm/objects/classes.hpp"

#include "hammer/vm/objects/function.hpp"
#include "hammer/vm/objects/hash_table.hpp"

namespace hammer::vm {

struct Method::Data : Header {
    Data()
        : Header(ValueType::Method) {}

    Value function;
};

size_t Method::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Method::walk(W&& w) {
    Data* d = access_heap();
    w(d->function);
}

Method::Data* Method::access_heap() const {
    return Value::access_heap<Data>();
}

struct DynamicObject::Data : Header {
    Data()
        : Header(ValueType::DynamicObject) {}

    HashTable properties;
};

size_t DynamicObject::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void DynamicObject::walk(W&& w) {
    Data* d = access_heap();
    w(d->properties);
}

DynamicObject::Data* DynamicObject::access_heap() const {
    return Value::access_heap<Data>();
}

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_CLASSES_IPP
