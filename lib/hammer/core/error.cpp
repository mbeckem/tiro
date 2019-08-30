#include "hammer/core/error.hpp"

namespace hammer {

Error::Error(std::string message)
    : message_(std::move(message)) {}

Error::~Error() {}

const char* Error::what() const noexcept {
    return message_.c_str();
}

void throw_internal_error(const char* file, int line, const char* function, std::string message) {
    std::string error_message = fmt::format("Internal error in {} ({}:{}): {}\n", function, file,
                                            line, message);
    throw Error(std::move(error_message));
}

} // namespace hammer
