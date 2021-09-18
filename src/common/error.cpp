#include "common/error.hpp"

namespace tiro {

namespace detail {

void throw_error_impl(
    [[maybe_unused]] const SourceLocation& loc, const char* format, fmt::format_args args) {

    fmt::memory_buffer buf;

#ifdef TIRO_DEBUG
    fmt::format_to(std::back_inserter(buf), "Internal error in {} ({}:{}): ", loc.function,
        loc.file, loc.line);
#endif
    fmt::vformat_to(std::back_inserter(buf), format, args);
    throw Error(to_string(buf));
}

} // namespace detail

Error::Error(std::string message)
    : message_(std::move(message)) {}

Error::~Error() {}

const char* Error::what() const noexcept {
    return message_.c_str();
}

} // namespace tiro