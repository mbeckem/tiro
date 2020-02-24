#ifndef TIRO_CORE_DEFS_HPP
#define TIRO_CORE_DEFS_HPP

#include <fmt/format.h>

#include <climits>
#include <cstdint>
#include <exception>
#include <string>
#include <type_traits>

namespace tiro {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

using byte = unsigned char;

using std::ptrdiff_t;
using std::size_t;
using std::uintptr_t;

#if defined(__GNUC__) || defined(__clang__)
#    define TIRO_LIKELY(x) (__builtin_expect(!!(x), 1))
#    define TIRO_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#    define TIRO_FORCE_INLINE inline __attribute__((always_inline))
#    define TIRO_DISABLE_INLINE __attribute__((noinline))
#    define TIRO_COLD __attribute__((cold))
#elif defined(_MSC_VER)
#    define TIRO_LIKELY(x) (x)
#    define TIRO_UNLIKELY(x) (x)
#    define TIRO_FORCE_INLINE inline __forceinline
#    define TIRO_DISABLE_INLINE __declspec(noinline)
#    define TIRO_COLD
#else
#    define TIRO_LIKELY(x) (x)
#    define TIRO_UNLIKELY(x) (x)
#    define TIRO_FORCE_INLINE inline
#    define TIRO_DISABLE_INLINE
#    define TIRO_COLD
#endif

/// Guards against weird platforms.
static_assert(
    CHAR_BIT == 8, "Bytes with a size other than 8 bits are not supported.");
static_assert(std::is_same_v<char, u8> || std::is_same_v<unsigned char, u8>,
    "uint8_t must be either char or unsigned char.");
static_assert(std::is_same_v<char, i8> || std::is_same_v<signed char, i8>,
    "int8_t must be either char or signed char.");
static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

struct SourceLocation {
    // Fields are 0 or NULL if compiled without debug symbols.
    const char* file;
    int line;
    const char* function;
};

// TODO: Own debugging macro
#ifndef NDEBUG
#    define TIRO_DEBUG
#endif

#ifdef TIRO_DEBUG
#    define TIRO_DEBUG_FILE __FILE__
#    define TIRO_DEBUG_LINE __LINE__
#    define TIRO_DEBUG_FUNC __func__
#else
#    define TIRO_DEBUG_FILE (nullptr)
#    define TIRO_DEBUG_LINE (0)
#    define TIRO_DEBUG_FUNC (nullptr)
#endif

#define TIRO_SOURCE_LOCATION() \
    (::tiro::SourceLocation{TIRO_DEBUG_FILE, TIRO_DEBUG_LINE, TIRO_DEBUG_FUNC})

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
#    define TIRO_ASSERT(cond, message)                         \
        do {                                                   \
            if (TIRO_UNLIKELY(!(cond))) {                      \
                ::tiro::detail::assert_fail(                   \
                    TIRO_SOURCE_LOCATION(), #cond, (message)); \
            }                                                  \
        } while (0)

/// Same as TIRO_ASSERT, but usable in constexpr functions.
#    define TIRO_CONSTEXPR_ASSERT(cond, message)               \
        do {                                                   \
            if (TIRO_UNLIKELY(!(cond))) {                      \
                throw ::tiro::detail::ConstexprAssertFail(     \
                    TIRO_SOURCE_LOCATION(), #cond, (message)); \
            }                                                  \
        } while (0)

/// Unconditionally terminate the program when unreachable code is executed.
#    define TIRO_UNREACHABLE(message) \
        (::tiro::detail::unreachable(TIRO_SOURCE_LOCATION(), (message)))

#else
#    define TIRO_ASSERT(cond, message)
#    define TIRO_CONSTEXPR_ASSERT(cond, message)
#    define TIRO_UNREACHABLE(message) \
        (::tiro::detail::unreachable(TIRO_SOURCE_LOCATION(), nullptr))
#endif

#define TIRO_ASSERT_NOT_NULL(pointer) \
    TIRO_ASSERT((pointer) != nullptr, #pointer " must not be null.")

/// Throws an internal error exception. The arguments to the macro are interpreted like in fmt::format().
#define TIRO_ERROR(...) \
    (::tiro::detail::throw_internal_error(TIRO_SOURCE_LOCATION(), __VA_ARGS__))

/// Evaluates a condition and, if the condition evaluates to false, throws an internal error.
/// All other arguments are passed to TIRO_ERROR().
#define TIRO_CHECK(cond, ...)         \
    do {                              \
        if (TIRO_UNLIKELY(!(cond))) { \
            TIRO_ERROR(__VA_ARGS__);  \
        }                             \
    } while (0)

/// Mark unimplemeted code parts.
#define TIRO_NOT_IMPLEMENTED() TIRO_UNREACHABLE("Not implemented yet.");

namespace detail {

struct ConstexprAssertFail {
    /// The constructor simply calls check_impl, this is part of the assertion implemention
    /// for constexpr functions.
    [[noreturn]] TIRO_DISABLE_INLINE TIRO_COLD ConstexprAssertFail(
        const SourceLocation& loc, const char* cond, const char* message);
};

[[noreturn]] TIRO_DISABLE_INLINE TIRO_COLD void throw_internal_error_impl(
    const SourceLocation& loc, const char* format, fmt::format_args args);

template<typename... Args>
[[noreturn]] TIRO_DISABLE_INLINE TIRO_COLD void throw_internal_error(
    const SourceLocation& loc, const char* format, const Args&... args) {
    throw_internal_error_impl(loc, format, fmt::make_format_args(args...));
}

[[noreturn]] TIRO_DISABLE_INLINE TIRO_COLD void
assert_fail(const SourceLocation& loc, const char* cond, const char* message);

[[noreturn]] TIRO_DISABLE_INLINE TIRO_COLD void
unreachable(const SourceLocation& loc, const char* message);

} // namespace detail

} // namespace tiro

#endif // TIRO_CORE_DEFS_HPP
