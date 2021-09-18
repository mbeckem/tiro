#ifndef TIRO_COMMON_ERROR_HPP
#define TIRO_COMMON_ERROR_HPP

#include "common/debug.hpp"
#include "common/defs.hpp"

#include <fmt/format.h>

#include <exception>
#include <string>

namespace tiro {

namespace detail {

[[noreturn]] TIRO_DISABLE_INLINE TIRO_COLD void
throw_error_impl(const SourceLocation& loc, const char* format, fmt::format_args args);

} // namespace detail

/// Error class thrown by the library when a fatal internal error occurs.
///
/// Normal (expected) errors (like syntax errors or runtime script errors) are reported
/// through other channels.
class Error : public virtual std::exception {
public:
    explicit Error(std::string message);
    virtual ~Error();

    virtual const char* what() const noexcept;

private:
    std::string message_;
};

/// Throws an internal error. The arguments to the macro are interpreted like in fmt::format().
#define TIRO_ERROR(...) (::tiro::throw_error(TIRO_SOURCE_LOCATION(), __VA_ARGS__))

/// Evaluates a condition and, if the condition evaluates to false, throws an internal error.
/// All other arguments are passed to TIRO_ERROR().
#define TIRO_CHECK(cond, ...)         \
    do {                              \
        if (TIRO_UNLIKELY(!(cond))) { \
            TIRO_ERROR(__VA_ARGS__);  \
        }                             \
    } while (0)

/// Throws an error with the provided source location.
template<typename... Args>
[[noreturn]] TIRO_DISABLE_INLINE TIRO_COLD void
throw_error(const SourceLocation& loc, const char* format, const Args&... args) {
    detail::throw_error_impl(loc, format, fmt::make_format_args(args...));
}

} // namespace tiro

#endif // TIRO_COMMON_ERROR_HPP