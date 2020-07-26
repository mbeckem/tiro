#include "api/internal.hpp"

namespace tiro::api {

char* copy_to_cstr(std::string_view str) {
    const size_t string_size = str.size();

    size_t alloc_size = string_size;
    if (!checked_add<size_t>(alloc_size, 1))
        throw std::bad_alloc();

    char* result = static_cast<char*>(::malloc(alloc_size));
    if (!result)
        throw std::bad_alloc();

    std::memcpy(result, str.data(), string_size);
    result[string_size] = '\0';
    return result;
}

} // namespace tiro::api
