#ifndef TIRO_VM_OBJECTS_NATIVE_OBJECTS_IPP
#define TIRO_VM_OBJECTS_NATIVE_OBJECTS_IPP

#include "vm/objects/native_objects.hpp"

#include <cstddef>

namespace tiro::vm {

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

template<typename W>
void NativeObject::walk(W&&) {}

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

template<typename W>
void NativePointer::walk(W&&) {}

NativePointer::Data* NativePointer::access_heap() const {
    return Value::access_heap<Data>();
}

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_NATIVE_OBJECTS_IPP
