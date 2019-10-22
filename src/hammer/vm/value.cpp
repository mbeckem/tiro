#include "hammer/vm/value.hpp"

#include "hammer/vm/coroutine.ipp"
#include "hammer/vm/object.ipp"

namespace hammer::vm {

std::string_view to_string(ValueType type) {
    switch (type) {
#define TYPE_TO_STR(Name) \
    case ValueType::Name: \
        return #Name;

        HAMMER_HEAP_TYPES(TYPE_TO_STR)

#undef TYPE_TO_STR
    }

    HAMMER_UNREACHABLE("Invalid value type.");
}

size_t Value::object_size() const noexcept {
    switch (type()) {

#define HANDLE_HEAP_TYPE(Name) \
    case ValueType::Name:      \
        return Name(*this).object_size();

        HAMMER_HEAP_TYPES(HANDLE_HEAP_TYPE)

#undef HANDLE_HEAP_TYPE
    }

    HAMMER_UNREACHABLE("Invalid value type.");
}

} // namespace hammer::vm
