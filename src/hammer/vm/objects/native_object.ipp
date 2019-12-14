#ifndef HAMMER_VM_OBJECTS_NATIVE_OBJECT_IPP
#define HAMMER_VM_OBJECTS_NATIVE_OBJECT_IPP

#include "hammer/vm/objects/native_object.hpp"

#include <cstddef>

namespace hammer::vm {

struct NativeObject::Data : Header {
    Data(size_t size_)
        : Header(ValueType::NativeObject)
        , size(size_)
        , cleanup(nullptr) {}

    // Linked list of finalizable objects.
    // Not walked! The collector uses this to discover
    // objects that must be finalized after marking.
    Value next_finalizer;

    size_t size;
    CleanupFn cleanup;
    alignas(std::max_align_t) byte data[];
};

size_t NativeObject::object_size() const noexcept {
    Data* d = access_heap();
    return sizeof(Data) + d->size;
}

NativeObject::Data* NativeObject::access_heap() const {
    return Value::access_heap<Data>();
}

struct NativePointer::Data : Header {
    Data()
        : Header(ValueType::NativePointer) {}

    void* pointer = nullptr;
};

size_t NativePointer::object_size() const noexcept {
    return sizeof(Data);
}

NativePointer::Data* NativePointer::access_heap() const {
    return Value::access_heap<Data>();
}

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_NATIVE_OBJECT_IPP
