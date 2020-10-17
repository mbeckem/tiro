#ifndef TIRO_COMMON_DEFS_HPP
#define TIRO_COMMON_DEFS_HPP

#include <cstdint>
#include <string_view>

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

using std::literals::string_view_literals::operator""sv;

#if defined(__GNUC__) || defined(__clang__)
#    define TIRO_LIKELY(x) (__builtin_expect(!!(x), 1))
#    define TIRO_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#    define TIRO_FORCE_INLINE inline __attribute__((always_inline))
#    define TIRO_DISABLE_INLINE __attribute__((noinline))
#    define TIRO_COLD __attribute__((cold))
#elif defined(_MSC_VER)
#    define TIRO_LIKELY(x) (!!(x))
#    define TIRO_UNLIKELY(x) (!!(x))
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

} // namespace tiro

#endif // TIRO_COMMON_DEFS_HPP
