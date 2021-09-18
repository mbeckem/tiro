#include "common/assert.hpp"

#include "common/error.hpp"

#include <fmt/format.h>

namespace tiro {

// #define TIRO_ABORT_ON_ASSERT_FAIL

/// Can be thrown on assertion failure. Most assertions are disabled in release builds.
/// Assertions can be configured to abort the process instead, but the default
/// is an exception being thrown.
class AssertionFailure final : public virtual Error {
public:
    explicit AssertionFailure(std::string message);
};

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
