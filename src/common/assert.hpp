#ifndef TIRO_COMMON_ASSERT_HPP
#define TIRO_COMMON_ASSERT_HPP

#include "common/debug.hpp"
#include "common/defs.hpp"

#include <string>
#include <string_view>

namespace tiro {

namespace detail {

[[noreturn]] TIRO_DISABLE_INLINE TIRO_COLD void
assert_fail(const SourceLocation& loc, const char* cond, const char* message);

[[noreturn]] TIRO_DISABLE_INLINE TIRO_COLD void
unreachable(const SourceLocation& loc, const char* message);

} // namespace detail

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

/// Mark unimplemented code parts.
#define TIRO_NOT_IMPLEMENTED() TIRO_ERROR("not implemented yet")

} // namespace tiro

#endif // TIRO_COMMON_ASSERT_HPP
