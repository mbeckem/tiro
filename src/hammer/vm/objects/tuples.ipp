#ifndef HAMMER_VM_OBJECTS_TUPLES_IPP
#define HAMMER_VM_OBJECTS_TUPLES_IPP

#include "hammer/vm/objects/tuples.hpp"

namespace hammer::vm {

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

#endif // HAMMER_VM_OBJECTS_TUPLES_IPP
