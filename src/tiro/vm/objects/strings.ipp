#ifndef TIRO_VM_OBJECTS_STRINGS_IPP
#define TIRO_VM_OBJECTS_STRINGS_IPP

#include "tiro/vm/objects/strings.hpp"

#include "tiro/vm/objects/arrays.hpp"
#include "tiro/vm/objects/buffers.hpp"

#include <fmt/format.h>

namespace tiro::vm {

struct String::Data : Header {
    Data(size_t size_)
        : Header(ValueType::String)
        , size(size_) {}

    size_t hash = 0; // Lazy
    size_t size;
    char data[];
};

size_t String::object_size() const noexcept {
    return sizeof(Data) + size();
}

template<typename W>
void String::walk(W&&) {}

String::Data* String::access_heap() const {
    return Value::access_heap<Data>();
}

struct StringBuilder::Data : Header {
    Data()
        : Header(ValueType::StringBuilder) {}

    size_t size = 0;
    Buffer buffer;
};

template<typename... Args>
void StringBuilder::format(Context& ctx, std::string_view fmt, Args&&... args) {
    Data* const d = access_heap();

    const size_t size = fmt::formatted_size(fmt, args...);
    if (size == 0)
        return;

    u8* buffer = reserve_free(d, ctx, size);
    fmt::format_to(buffer, fmt, args...);
    d->size += size;
}

size_t StringBuilder::object_size() const noexcept {
    return sizeof(Data);
}

template<typename W>
void StringBuilder::walk(W&& w) {
    Data* d = access_heap();
    w(d->buffer);
}

StringBuilder::Data* StringBuilder::access_heap() const {
    return Value::access_heap<Data>();
}

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_STRINGS_IPP
