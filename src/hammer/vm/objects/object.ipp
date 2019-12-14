#ifndef HAMMER_VM_OBJECTS_OBJECT_IPP
#define HAMMER_VM_OBJECTS_OBJECT_IPP

#include "hammer/vm/objects/object.hpp"

#include "hammer/vm/objects/strings.hpp"

namespace hammer::vm {

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

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_OBJECT_IPP
