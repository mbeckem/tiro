#ifndef HAMMER_VM_OBJECTS_BUFFERS_IPP
#define HAMMER_VM_OBJECTS_BUFFERS_IPP

#include "hammer/vm/objects/buffers.hpp"

#include <cstddef>

namespace hammer::vm {

struct Buffer::Data : Header {
    Data(size_t size_)
        : Header(ValueType::Buffer)
        , size(size_) {}

    size_t size;
    alignas(std::max_align_t) byte values[];
};

size_t Buffer::object_size() const noexcept {
    return sizeof(Data) + size();
}

Buffer::Data* Buffer::access_heap() const {
    return Value::access_heap<Data>();
}

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_BUFFERS_IPP
