#include "common/error.hpp"

namespace tiro {

namespace detail {

void throw_error_impl([[maybe_unused]] const SourceLocation& loc, tiro_errc_t code,
    const char* format, fmt::format_args args) {

    fmt::memory_buffer buf;

#ifdef TIRO_DEBUG
    fmt::format_to(
        std::back_inserter(buf), "Error in {} ({}:{}): ", loc.function, loc.file, loc.line);
#endif

    fmt::vformat_to(std::back_inserter(buf), format, args);
    throw Error(code, to_string(buf));
}

} // namespace detail

Error::Error(tiro_errc_t code, std::string message)
    : code_(code)
    , message_(std::move(message)) {}

Error::~Error() {}

tiro_errc_t Error::code() const noexcept {
    return code_;
}

const char* Error::what() const noexcept {
    return message_.c_str();
}

} // namespace tiro
