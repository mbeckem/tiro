#ifndef TIRO_VM_OBJECTS_CLASSES_IPP
#define TIRO_VM_OBJECTS_CLASSES_IPP

#include "tiro/vm/objects/classes.hpp"

#include "tiro/vm/objects/functions.hpp"
#include "tiro/vm/objects/hash_tables.hpp"
#include "tiro/vm/objects/strings.hpp"

namespace tiro::vm {

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

struct Symbol::Data : Header {
    Data(String name_)
        : Header(ValueType::Symbol)
        , name(name_) {}

    String name;
};

size_t Symbol::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Symbol::walk(W&& w) {
    Data* d = access_heap();
    w(d->name);
}

Symbol::Data* Symbol::access_heap() const {
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

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_CLASSES_IPP
