#ifndef HAMMER_VM_OBJECTS_OBJECT_IPP
#define HAMMER_VM_OBJECTS_OBJECT_IPP

#include "hammer/vm/objects/object.hpp"

#include "hammer/vm/objects/string.hpp"

namespace hammer::vm {

struct Undefined::Data : Header {
    Data()
        : Header(ValueType::Undefined) {}
};

size_t Undefined::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Undefined::walk(W&&) {}

struct Boolean::Data : Header {
    Data(bool v)
        : Header(ValueType::Boolean)
        , value(v) {}

    bool value;
};

size_t Boolean::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Boolean::walk(W&&) {}

struct Integer::Data : Header {
    Data(i64 v)
        : Header(ValueType::Integer)
        , value(v) {}

    i64 value;
};

size_t Integer::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Integer::walk(W&&) {}

struct Float::Data : Header {
    Data(f64 v)
        : Header(ValueType::Float)
        , value(v) {}

    f64 value;
};

size_t Float::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
inline void Float::walk(W&&) {}

struct SpecialValue::Data : Header {
    Data(String name_)
        : Header(ValueType::SpecialValue)
        , name(name_) {}

    String name;
};

size_t SpecialValue::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void SpecialValue::walk(W&& w) {
    Data* d = access_heap();
    w(d->name);
}

SpecialValue::Data* SpecialValue::access_heap() const {
    return Value::access_heap<Data>();
}

struct Module::Data : Header {
    Data(String name_, Tuple members_)
        : Header(ValueType::Module)
        , name(name_)
        , members(members_) {}

    String name;
    Tuple members;
};

size_t Module::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Module::walk(W&& w) {
    Data* d = access_heap<Data>();
    w(d->name);
    w(d->members);
}

struct Tuple::Data : Header {
    template<typename Init>
    Data(size_t size_, Init&& init)
        : Header(ValueType::Tuple)
        , size(size_) {
        init(this);
    }

    size_t size;
    Value values[];
};

size_t Tuple::object_size() const noexcept {
    return sizeof(Data) + size() * sizeof(Value);
}

template<typename W>
void Tuple::walk(W&& w) {
    Data* d = access_heap();
    w.array(ArrayVisitor(d->values, d->size));
}

Tuple::Data* Tuple::access_heap() const {
    return Value::access_heap<Data>();
}

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_OBJECT_IPP
