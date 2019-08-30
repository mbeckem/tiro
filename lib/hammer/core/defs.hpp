#ifndef HAMMER_COMMON_DEFS_HPP
#define HAMMER_COMMON_DEFS_HPP

#include <climits>
#include <cstdint>
#include <type_traits>

namespace hammer {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using byte = unsigned char;

using std::ptrdiff_t;
using std::size_t;
using std::uintptr_t;

#if defined(__GNUC__) || defined(__clang__)
#    define HAMMER_LIKELY(x) (__builtin_expect(!!(x), 1))
#    define HAMMER_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#    define HAMMER_FORCE_INLINE inline __attribute__((always_inline))
#    define HAMMER_DISABLE_INLINE __attribute__((noinline))
#    define HAMMER_COLD __attribute__((cold))
#elif defined(_MSC_VER)
#    define HAMMER_LIKELY(x) (x)
#    define HAMMER_UNLIKELY(x) (x)
#    define HAMMER_FORCE_INLINE inline __forceinline
#    define HAMMER_DISABLE_INLINE __declspec(noinline)
#    define HAMMER_COLD // TODO
#else
#    define HAMMER_LIKELY(x) (x)
#    define HAMMER_UNLIKELY(x) (x)
#    define HAMMER_FORCE_INLINE inline
#    define HAMMER_DISABLE_INLINE
#    define HAMMER_COLD
#endif

/*
 * Guards against weird platforms.
 */
static_assert(CHAR_BIT == 8, "Bytes with a size other than 8 bits are not supported.");
static_assert(std::is_same_v<char, u8> || std::is_same_v<unsigned char, u8>,
              "uint8_t must be either char or unsigned char.");
static_assert(std::is_same_v<char, i8> || std::is_same_v<signed char, i8>,
              "int8_t must be either char or signed char.");

/**
 * Suppresses "unused" warnings. Does nothing with its arguments.
 */
template<typename... Args>
void unused(Args&&...) {}

// TODO: Own debugging macro
#ifndef NDEBUG
#    define HAMMER_DEBUG
#endif

#ifdef HAMMER_DEBUG

/**
 * When in debug mode, check against the given condition
 * and abort the program with a message if the check fails.
 * Does nothing in release mode.
 */
#    define HAMMER_ASSERT(cond, message)                                             \
        do {                                                                         \
            if (HAMMER_UNLIKELY(!(cond))) {                                          \
                ::hammer::detail::assert_fail(__FILE__, __LINE__, #cond, (message)); \
            }                                                                        \
        } while (0)

/**
 * Same as HAMMER_ASSERT, but usable in constexpr functions.
 */
#    define HAMMER_CONSTEXPR_ASSERT(cond, message)                                       \
        do {                                                                             \
            if (HAMMER_UNLIKELY(!(cond))) {                                              \
                throw ::hammer::detail::constexpr_assert_fail(__FILE__, __LINE__, #cond, \
                                                              (message));                \
            }                                                                            \
        } while (0)

#else

#    define HAMMER_ASSERT(cond, message)

#    define HAMMER_CONSTEXPR_ASSERT(cond, message)

#endif

#define HAMMER_ASSERT_NOT_NULL(pointer) HAMMER_ASSERT((pointer), #pointer " must not be null.")

/**
 * Unconditionally terminate the program when unreachable code is executed.
 */
#define HAMMER_UNREACHABLE(message) (::hammer::detail::unreachable(__FILE__, __LINE__, (message)))

/**
 * Mark unimplemeted code parts.
 */
#define HAMMER_NOT_IMPLEMENTED() HAMMER_UNREACHABLE("Not implemented yet.");

namespace detail {

struct constexpr_assert_fail {
    /// The constructor simply calls check_impl, this is part of the assertion implemention
    /// for constexpr functions.
    HAMMER_DISABLE_INLINE HAMMER_COLD constexpr_assert_fail(const char* file, int line,
                                                            const char* cond, const char* message);
};

[[noreturn]] HAMMER_DISABLE_INLINE HAMMER_COLD void
assert_fail(const char* file, int line, const char* cond, const char* message);
[[noreturn]] HAMMER_DISABLE_INLINE HAMMER_COLD void unreachable(const char* file, int line,
                                                                const char* message);
[[noreturn]] HAMMER_DISABLE_INLINE HAMMER_COLD void abort(const char* file, int line,
                                                          const char* message);

} // namespace detail

} // namespace hammer

#endif // HAMMER_COMMON_DEFS_HPP
