#ifndef TIRO_CORE_MATH_HPP
#define TIRO_CORE_MATH_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/type_traits.hpp"

#include <limits>

namespace tiro {

template<typename T>
using IsUnsigned = std::enable_if_t<std::is_unsigned<T>::value, T>;

template<typename T>
using IsInteger = std::enable_if_t<std::is_integral<T>::value, T>;

/// Returns true if the index range [offset, offset + n) is valid for a datastructure of the given `size`.
template<typename T, IsUnsigned<T>* = nullptr>
constexpr bool range_in_bounds(T size, T offset, T n) {
    return offset <= size && n <= size - offset;
}

/// Returns the number of bits in the representation of T.
template<typename T, IsUnsigned<T>* = nullptr>
constexpr size_t type_bits() noexcept {
    static_assert(CHAR_BIT == 8, "Unexpected number of bits in char.");
    return sizeof(T) * CHAR_BIT;
}

/// Computes the base-2 logarithm of `v`.
/// \pre `v > 0`.
template<typename T, IsUnsigned<T>* = nullptr>
constexpr T log2(T v) noexcept {
    TIRO_CONSTEXPR_ASSERT(v != 0, "v must be greater than zero.");
    T log = 0;
    while (v >>= 1)
        ++log;
    return log;
}

/// Returns true if the given integer is a power of two.
template<typename T, IsUnsigned<T>* = nullptr>
constexpr bool is_pow2(T v) noexcept {
    return v && !(v & (v - 1));
}

/// Returns the largest power of two representable by T.
template<typename T, IsUnsigned<T>* = nullptr>
constexpr T max_pow2() noexcept {
    constexpr T max_log2 = log2(std::numeric_limits<T>::max());
    return T(1) << max_log2;
}

/// Rounds `v` towards the next power of two. Returns `v` if it is already a power of two.
/// Note: returns 0 if `v == 0`.
///
/// Adapted from http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
template<typename T, IsUnsigned<T>* = nullptr>
constexpr T ceil_pow2(T v) noexcept {
    TIRO_CONSTEXPR_ASSERT(v <= max_pow2<T>(),
        "Cannot ceil to pow2 for values that are larger than the maximum power "
        "of two.");

    --v;
    for (u64 i = 1; i < sizeof(T) * CHAR_BIT; i *= 2) {
        v |= v >> i;
    }
    return ++v;
}

/// Returns the index of the largest bit that can be set in the given type.
/// I.e. max_bit<u32>() == 31.
template<typename T, IsUnsigned<T>* = nullptr>
constexpr T max_bit() noexcept {
    return log2(std::numeric_limits<T>::max());
}

/// Rounds `a` towards the next multiple of `b`.
/// `b` must not be 0.
template<typename T, IsUnsigned<T>* = nullptr>
constexpr T ceil(T a, T b) noexcept {
    TIRO_CONSTEXPR_ASSERT(b != 0, "b must not be 0.");
    return ((a + b - 1) / b) * b;
}

/// Returns a % b where b is a power of two.
template<typename T, IsUnsigned<T>* = nullptr>
constexpr T mod_pow2(T a, T b) noexcept {
    TIRO_CONSTEXPR_ASSERT(is_pow2(b), "b must be a power of two");
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
    TIRO_CONSTEXPR_ASSERT(
        a >= 0, "Dividend must be greater than or equal to zero.");
    TIRO_CONSTEXPR_ASSERT(b > 0, "Divisor must be greater than zero.");
    return (a + b - 1) / b;
}

// TODO: Support other compilers. Also make a fallback with "normal" conditions.
// Clean up the api mess below (value returns / reference arguments).

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

template<typename To, typename From, IsInteger<From>* = nullptr,
    IsInteger<To>* = nullptr>
To checked_cast(From from) {
    using to_limits = std::numeric_limits<To>;

    // https://wiki.sei.cmu.edu/confluence/display/c/INT31-C.+Ensure+that+integer+conversions+do+not+result+in+lost+or+misinterpreted+data
    if constexpr (std::is_unsigned_v<From>) {
        // If To is signed, it will be promoted to unsigned.
        // If it is unsigned, the comparison is safe anyway.
        TIRO_CHECK(from <= std::make_unsigned_t<To>(to_limits::max()),
            "Integer cast failed (overflow).");
        return static_cast<To>(from);
    }
    // From is signed
    else if constexpr (std::is_unsigned_v<To>) {
        // The promotion of from to unsigned in the second check should make this safe.
        TIRO_CHECK(
            from >= 0 && std::make_unsigned_t<From>(from) <= to_limits::max(),
            "Integer cast failed (overflow).");
        return static_cast<To>(from);
    }
    // Both are signed
    else {
        TIRO_CHECK(from >= to_limits::min() && from <= to_limits::max(),
            "Integer cast failed (overflow).");
        return static_cast<To>(from);
    }
}

} // namespace tiro

#endif // TIRO_CORE_MATH_HPP
