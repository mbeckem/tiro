#ifndef TIRO_COMMON_BITOPS_HPP
#define TIRO_COMMON_BITOPS_HPP

#include "common/defs.hpp"

// See also C++20 header <bit>
namespace tiro {

namespace detail {

#if defined(__GNUC__) || defined(__clang__)

inline int popcount(unsigned int v) {
    return __builtin_popcount(v);
}

inline int popcount(unsigned long v) {
    return __builtin_popcountl(v);
}

inline int popcount(unsigned long long v) {
    return __builtin_popcountll(v);
}

inline int ffs(unsigned int v) {
    return __builtin_ffs(v);
}

inline int ffs(unsigned long v) {
    return __builtin_ffsl(v);
}

inline int ffs(unsigned long long v) {
    return __builtin_ffsll(v);
}

inline int clz(unsigned int v) {
    return __builtin_clz(v);
}

inline int clz(unsigned long v) {
    return __builtin_clzl(v);
}

inline int clz(unsigned long long v) {
    return __builtin_clzll(v);
}

// Disable implicit conversions
template<typename T>
int popcount(T t) = delete;

template<typename T>
int ffs(T t) = delete;

template<typename T>
int clz(T t) = delete;

#endif // defined(__GNUC__) || defined(__clang__)

} // namespace detail

/// Returns the number of bits set in `v`.
template<typename Unsigned>
int popcount(Unsigned v) {
    return detail::popcount(v);
}

/// Returns one plus the index of the least significant 1-bit of `v`, or if x is zero, returns zero.
template<typename Unsigned>
int find_first_set(Unsigned v) {
    return detail::ffs(v);
}

/// Returns the number of leading (most significant) zeroes in `v`.
/// \pre `v != 0`.
template<typename Unsigned>
int count_leading_zeroes(Unsigned v) {
    return detail::clz(v);
}

} // namespace tiro

#endif // TIRO_COMMON_BITOPS_HPP
