#ifndef HAMMER_COMMON_MATH_HPP
#define HAMMER_COMMON_MATH_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/error.hpp"
#include "hammer/core/type_traits.hpp"

namespace hammer {

template<typename T>
using IsUnsigned = std::enable_if_t<std::is_unsigned<T>::value, T>;

template<typename T>
using IsInteger = std::enable_if_t<std::is_integral<T>::value, T>;

/// Returns true if the index range [offset, offset + n) is valid for a datastructure of the given `size`.
template<typename T, IsUnsigned<T>* = nullptr>
constexpr bool range_in_bounds(T size, T offset, T n) {
    return offset <= size && n <= size - offset;
}

/// Rounds `v` towards the next power of two. Returns `v` if it is already a power of two.
/// Note: returns 0 if `v == 0`.
///
/// Adapted from http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
template<typename T, IsUnsigned<T>* = nullptr>
constexpr T ceil_pow2(T v) noexcept {
    --v;
    for (u64 i = 1; i < sizeof(T) * CHAR_BIT; i *= 2) {
        v |= v >> i;
    }
    return ++v;
}

/// Rounds `a` towards the next multiple of `b`.
/// `b` must not be 0.
template<typename T, IsUnsigned<T>* = nullptr>
constexpr T ceil(T a, T b) noexcept {
    HAMMER_CONSTEXPR_ASSERT(b != 0, "b must not be 0.");
    return ((a + b - 1) / b) * b;
}

/// Returns true if the given integer is a power of two.
template<typename T, IsUnsigned<T>* = nullptr>
constexpr bool is_pow2(T v) noexcept {
    return v && !(v & (v - 1));
}

/// Returns a % b where b is a power of two.
template<typename T, IsUnsigned<T>* = nullptr>
constexpr T mod_pow2(T a, T b) noexcept {
    HAMMER_CONSTEXPR_ASSERT(is_pow2(b), "b must be a power of two");
    return a & (b - 1);
}

/// Returns true if a is aligned, i.e. if it is divisible by b.
/// b must be a power of two.
template<typename T, IsUnsigned<T>* = nullptr>
constexpr bool is_aligned(T a, T b) noexcept {
    return mod_pow2(a, b) == 0;
}

/// Returns ceil(A / B) for two positive (non-zero) integers.
template<typename T, IsInteger<T>* = nullptr>
constexpr T ceil_div(T a, T b) {
    HAMMER_CONSTEXPR_ASSERT(a >= 0, "Dividend must be greater than or equal to zero.");
    HAMMER_CONSTEXPR_ASSERT(b > 0, "Divisor must be greater than zero.");
    return (a + b - 1) / b;
}

/*
 * TODO: Support other compilers. Also make a fallback with "normal" conditions.
 * Clean up the api mess below (value returns / reference arguments).
 */

/// Computes out = a + b.
/// Returns false if the addition `a+b` overflowed.
template<typename T, IsInteger<T>* = nullptr>
constexpr bool checked_add(T a, T b, T& out) {
    return !__builtin_add_overflow(a, b, &out);
}

/// Computes a += b.
/// Returns false if the addition `a+b` overflowed.
template<typename T, IsInteger<T>* = nullptr>
constexpr bool checked_add(T& a, T b) {
    return checked_add(a, b, a);
}

/// Computes out = a - b;
/// Returns false if the subtraction `a-b` overflowed.
template<typename T, IsInteger<T>* = nullptr>
constexpr bool checked_sub(T a, T b, T& out) {
    return !__builtin_sub_overflow(a, b, &out);
}

/// Computes a -= b;
/// Returns false if the subtraction `a-b` overflowed.
template<typename T, IsInteger<T>* = nullptr>
constexpr bool checked_sub(T& a, T b) {
    return checked_sub(a, b, a);
}

/// Computes out = a * b;
/// Returns false if the multiplication `a*b` overflowed.
template<typename T, IsInteger<T>* = nullptr>
constexpr bool checked_mul(T a, T b, T& out) {
    return !__builtin_mul_overflow(a, b, &out);
}

/// Computes a *= b;
/// Returns false if the multiplication `a*b` overflowed.
template<typename T, IsInteger<T>* = nullptr>
constexpr bool checked_mul(T& a, T b) {
    return checked_mul(a, b, a);
}

} // namespace hammer

#endif // HAMMER_COMMON_MATH_HPP
