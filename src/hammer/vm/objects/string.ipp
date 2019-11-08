#ifndef HAMMER_VM_OBJECTS_STRING_IPP
#define HAMMER_VM_OBJECTS_STRING_IPP

#include "hammer/vm/objects/string.hpp"

namespace hammer::vm {

struct String::Data : Header {
    Data(std::string_view str)
        : Header(ValueType::String)
        , size(str.size()) {
        std::memcpy(data, str.data(), str.size());
    }

    size_t hash = 0; // Lazy
    size_t size;
    char data[];
};

size_t String::object_size() const noexcept {
    return sizeof(Data) + size();
}

template<typename W>
void String::walk(W&&) {}

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_STRING_IPP
