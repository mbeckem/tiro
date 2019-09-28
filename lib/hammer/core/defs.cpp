#include "hammer/core/defs.hpp"

#include <fmt/format.h>

#include <cstring>
#include <iostream>

namespace hammer {

Error::Error(std::string message)
    : message_(std::move(message)) {}

Error::~Error() {}

const char* Error::what() const noexcept {
    return message_.c_str();
}

AssertionFailure::AssertionFailure(std::string message)
    : Error(std::move(message)){}

          [[noreturn]] static void abort_impl(std::string message) {
#ifdef HAMMER_ABORT_ON_ASSERT_FAIL
    std::cerr << message << std::endl;
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

void throw_internal_error(
    const char* file, int line, const char* function, std::string message) {
    std::string error_message = fmt::format(
        "Internal error in {} ({}:{}): {}\n", function, file, line, message);
    throw Error(std::move(error_message));
}

void assert_fail(
    const char* file, int line, const char* condition, const char* message) {

    fmt::memory_buffer buf;
    fmt::format_to(buf, "Assertion `{}` failed", condition);
    if (message && std::strlen(message) > 0) {
        fmt::format_to(buf, ": {}", message);
    }

    fmt::format_to(buf, "\n");
    fmt::format_to(buf, "    (in {}:{})", file, line);
    abort_impl(to_string(buf));
}

void unreachable(const char* file, int line, const char* message) {
    fmt::memory_buffer buf;
    fmt::format_to(buf, "Unreachable code executed");
    if (message && std::strlen(message) > 0) {
        fmt::format_to(buf, ": {}", message);
    }

    fmt::format_to(buf, "\n");
    fmt::format_to(buf, "    (in {}:{})", file, line);
    abort_impl(to_string(buf));
}

} // namespace detail
} // namespace hammer
