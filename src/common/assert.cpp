#include "common/assert.hpp"

namespace tiro {

// #define TIRO_ABORT_ON_ASSERT_FAIL

Error::Error(std::string message)
    : message_(std::move(message)) {}

Error::~Error() {}

const char* Error::what() const noexcept {
    return message_.c_str();
}

AssertionFailure::AssertionFailure(std::string message)
    : Error(std::move(message)) {}

[[noreturn]] static void throw_or_abort(std::string message) {
#ifdef TIRO_ABORT_ON_ASSERT_FAIL
    fmt::print(stderr, "{}\n", message);
    std::fflush(stderr);
    std::abort();
#else
    throw AssertionFailure(std::move(message));
#endif
}

namespace detail {

void throw_internal_error_impl(
    [[maybe_unused]] const SourceLocation& loc, const char* format, fmt::format_args args) {

    fmt::memory_buffer buf;

#ifdef TIRO_DEBUG
    fmt::format_to(std::back_inserter(buf), "Internal error in {} ({}:{}): ", loc.function,
        loc.file, loc.line);
#endif
    fmt::vformat_to(std::back_inserter(buf), format, args);
    throw Error(to_string(buf));
}

void assert_fail(
    [[maybe_unused]] const SourceLocation& loc, const char* condition, const char* message) {

    fmt::memory_buffer buf;
    fmt::format_to(std::back_inserter(buf), "Assertion `{}` failed", condition);
    if (message && std::strlen(message) > 0) {
        fmt::format_to(std::back_inserter(buf), ": {}", message);
    }

#ifdef TIRO_DEBUG
    fmt::format_to(std::back_inserter(buf), "\n");
    fmt::format_to(std::back_inserter(buf), "    (in {}:{})", loc.file, loc.line);
#endif

    throw_or_abort(to_string(buf));
}

void unreachable([[maybe_unused]] const SourceLocation& loc, const char* message) {
    fmt::memory_buffer buf;
    fmt::format_to(std::back_inserter(buf), "Unreachable code executed");
    if (message && std::strlen(message) > 0) {
        fmt::format_to(std::back_inserter(buf), ": {}", message);
    }

#ifdef TIRO_DEBUG
    fmt::format_to(std::back_inserter(buf), "\n");
    fmt::format_to(std::back_inserter(buf), "    (in {}:{})", loc.file, loc.line);
#endif
    throw_or_abort(to_string(buf));
}

} // namespace detail
} // namespace tiro
