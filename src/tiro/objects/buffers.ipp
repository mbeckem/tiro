#ifndef TIRO_OBJECTS_BUFFERS_IPP
#define TIRO_OBJECTS_BUFFERS_IPP

#include "tiro/objects/buffers.hpp"

#include <cstddef>

namespace tiro::vm {

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

} // namespace tiro::vm

#endif // TIRO_OBJECTS_BUFFERS_IPP
