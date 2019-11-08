#ifndef HAMMER_VM_OBJECTS_ARRAY_IPP
#define HAMMER_VM_OBJECTS_ARRAY_IPP

#include "hammer/vm/objects/array.hpp"

#include "hammer/vm/context.hpp"

#include "hammer/vm/objects/array_storage_base.ipp"

namespace hammer::vm {

struct Array::Data : Header {
    Data()
        : Header(ValueType::Array) {}

    ArrayStorage storage;
};

size_t Array::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void Array::walk(W&& w) {
    Data* d = access_heap();
    w(d->storage);
}

Array::Data* Array::access_heap() const {
    return Value::access_heap<Data>();
}

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_ARRAY_IPP
