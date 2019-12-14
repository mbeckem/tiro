#ifndef HAMMER_VM_OBJECTS_PRIMITIVES_IPP
#define HAMMER_VM_OBJECTS_PRIMITIVES_IPP

#include "hammer/vm/objects/primitives.hpp"

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

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_PRIMITIVES_IPP
