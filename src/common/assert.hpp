#ifndef TIRO_COMMON_ASSERT_HPP
#define TIRO_COMMON_ASSERT_HPP

#include "common/defs.hpp"

#include <fmt/format.h>

#include <exception>
#include <string>
#include <string_view>

namespace tiro {

struct SourceLocation {
    // Fields are 0 or NULL if compiled without debug symbols.
    const char* file = nullptr;
    int line = 0;
    const char* function = nullptr;

    constexpr SourceLocation() = default;

    constexpr SourceLocation(const char* file_, int line_, const char* function_)
        : file(file_)
        , line(line_)
        , function(function_) {}

    explicit constexpr operator bool() const { return file != nullptr; }
};

inline constexpr SourceLocation source_location_unavailable{};

#if !defined(TIRO_DEBUG) && !defined(NDEBUG)
#    define TIRO_DEBUG 1
#endif

#ifdef TIRO_DEBUG
#    define TIRO_SOURCE_LOCATION() (::tiro::SourceLocation{__FILE__, __LINE__, __func__})
#else
#    define TIRO_SOURCE_LOCATION() (::tiro::source_location_unavailable)
#endif

namespace detail {

[[noreturn]] TIRO_DISABLE_INLINE TIRO_COLD void
throw_internal_error_impl(const SourceLocation& loc, const char* format, fmt::format_args args);

[[noreturn]] TIRO_DISABLE_INLINE TIRO_COLD void
assert_fail(const SourceLocation& loc, const char* cond, const char* message);

[[noreturn]] TIRO_DISABLE_INLINE TIRO_COLD void
unreachable(const SourceLocation& loc, const char* message);

} // namespace detail

/// Error class thrown by the library when a fatal internal error occurs.
///
/// Normal errors (like syntax errors or runtime script errors) are reported
/// through other channels.
class Error : public virtual std::exception {
public:
    explicit Error(std::string message);
    virtual ~Error();

    virtual const char* what() const noexcept;

private:
    std::string message_;
};

/// Can be thrown on assertion failure. Most assertions are disabled in release builds.
/// Assertions can be configured to abort the process instead, but the default
/// is an exception being thrown.
class AssertionFailure final : public virtual Error {
public:
    explicit AssertionFailure(std::string message);
};

#ifdef TIRO_DEBUG

/// When in debug mode, check against the given condition
/// and abort the program with a message if the check fails.
/// Does nothing in release mode.
/// Technique inspired by https://akrzemi1.wordpress.com/2017/05/18/asserts-in-constexpr-functions/
#    define TIRO_DEBUG_ASSERT(cond, message)                            \
        do {                                                            \
            if (TIRO_UNLIKELY(!(cond))) {                               \
                [loc = TIRO_SOURCE_LOCATION()] {                        \
                    ::tiro::detail::assert_fail(loc, #cond, (message)); \
                }();                                                    \
            }                                                           \
        } while (0)

/// Unconditionally terminate the program when unreachable code is executed.
#    define TIRO_UNREACHABLE(message) \
        (::tiro::detail::unreachable(TIRO_SOURCE_LOCATION(), (message)))

#else
#    define TIRO_DEBUG_ASSERT(cond, message)
#    define TIRO_UNREACHABLE(message) (::tiro::detail::unreachable(TIRO_SOURCE_LOCATION(), nullptr))
#endif

/// Throws an internal error. The arguments to the macro are interpreted like in fmt::format().
#define TIRO_ERROR(...) (::tiro::throw_internal_error(TIRO_SOURCE_LOCATION(), __VA_ARGS__))

/// Evaluates a condition and, if the condition evaluates to false, throws an internal error.
/// All other arguments are passed to TIRO_ERROR().
#define TIRO_CHECK(cond, ...)         \
    do {                              \
        if (TIRO_UNLIKELY(!(cond))) { \
            TIRO_ERROR(__VA_ARGS__);  \
        }                             \
    } while (0)

/// Mark unimplemented code parts.
#define TIRO_NOT_IMPLEMENTED() TIRO_ERROR("Not implemented yet.")

/// Throws an error with the provided source location.
// TODO: Better error api for multiple error types.
template<typename... Args>
[[noreturn]] TIRO_DISABLE_INLINE TIRO_COLD void
throw_internal_error(const SourceLocation& loc, const char* format, const Args&... args) {
    detail::throw_internal_error_impl(loc, format, fmt::make_format_args(args...));
}

} // namespace tiro

#endif // TIRO_COMMON_ASSERT_HPP
