#include "tiro/core/defs.hpp"

#include <fmt/format.h>

#include <cstring>

namespace tiro {

// #define TIRO_ABORT_ON_ASSERT_FAIL

Error::Error(std::string message)
    : message_(std::move(message)) {}

Error::~Error() {}

const char* Error::what() const noexcept {
    return message_.c_str();
}

AssertionFailure::AssertionFailure(std::string message)
    : Error(std::move(message)){}

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

ConstexprAssertFail::ConstexprAssertFail(
    const char* file, int line, const char* cond, const char* message) {
    assert_fail(file, line, cond, message);
}

void throw_internal_error([[maybe_unused]] const char* file,
    [[maybe_unused]] int line, [[maybe_unused]] const char* function,
    std::string message) {

    std::string error_message =
#ifdef TIRO_DEBUG
        fmt::format(
            "Internal error in {} ({}:{}): {}", function, file, line, message);
#else
        std::move(message);
#endif
    throw Error(std::move(error_message));
}

void assert_fail([[maybe_unused]] const char* file, [[maybe_unused]] int line,
    const char* condition, const char* message) {

    fmt::memory_buffer buf;
    fmt::format_to(buf, "Assertion `{}` failed", condition);
    if (message && std::strlen(message) > 0) {
        fmt::format_to(buf, ": {}", message);
    }

#ifdef TIRO_DEBUG
    fmt::format_to(buf, "\n");
    fmt::format_to(buf, "    (in {}:{})", file, line);
#endif
    throw_or_abort(to_string(buf));
}

void unreachable([[maybe_unused]] const char* file, [[maybe_unused]] int line,
    const char* message) {
    fmt::memory_buffer buf;
    fmt::format_to(buf, "Unreachable code executed");
    if (message && std::strlen(message) > 0) {
        fmt::format_to(buf, ": {}", message);
    }

#ifdef TIRO_DEBUG
    fmt::format_to(buf, "\n");
    fmt::format_to(buf, "    (in {}:{})", file, line);
#endif
    throw_or_abort(to_string(buf));
}

} // namespace detail
} // namespace tiro
