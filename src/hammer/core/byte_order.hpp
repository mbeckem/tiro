#ifndef HAMMER_CORE_BYTE_ORDER_HPP
#define HAMMER_CORE_BYTE_ORDER_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/span.hpp"

#ifdef _MSC_VER
#    include <stdlib.h> // for byteswaps
#endif

namespace hammer {

/// The possible values for the order of bytes within the binary representation of an integer.
enum class ByteOrder {
    BigEndian,    // Most significant byte in lowest memory address
    LittleEndian, // Most significant byte in highest memory address
};

// __BYTE_ORDER__ is gcc
#if defined(__BYTE_ORDER__)
#    if defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#        define HAMMER_BYTE_ORDER ByteOrder::BigEndian
#    elif defined(__ORDER_LITTLE_ENDIAN__) \
        && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#        define HAMMER_BYTE_ORDER ByteOrder::LittleEndian
#    else
#        error Unsupported byte order.
#    endif
// __{*}_ENDIAN__ is clang
#elif defined(__BIG_ENDIAN__)
#    define HAMMER_BYTE_ORDER ByteOrder::BigEndian
#elif defined(__LITTLE_ENDIAN__)
#    define HAMMER_BYTE_ORDER ByteOrder::LittleEndian
// This should cover win32 and any other x86 machines
#elif defined(_MSC_VER) || defined(__i386__) || defined(__x86_64__)
#    define HAMMER_BYTE_ORDER ByteOrder::LittleEndian
#else
#    error Failed to detect byte order.
#endif

/// Holds the native byte order of the host.
inline constexpr ByteOrder host_byte_order = HAMMER_BYTE_ORDER;

constexpr u16 byteswap(u16 v) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(v);
#elif defined(_MSC_VER)
    return _byteswap_ushort(v);
#else
#    error No byteswap implementation for this platform as of yet.
#endif
}

constexpr u32 byteswap(u32 v) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(v);
#elif defined(_MSC_VER)
    return _byteswap_ulong(v);
#else
#    error No byteswap implementation for this platform as of yet.
#endif
}

constexpr u64 byteswap(u64 v) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(v);
#elif defined(_MSC_VER)
    return _byteswap_uint64(v);
#else
#    error No byteswap implementation for this platform as of yet.
#endif
}

template<typename T>
constexpr bool is_swappable_integer =
    std::is_same_v<T, u16> || std::is_same_v<T, u32> || std::is_same_v<T, u64>;

/// Returns `v` converted from byte order `from` to byte order `to`.
template<ByteOrder from, ByteOrder to, typename T>
T convert_byte_order(T v) {
    if constexpr (sizeof(v) == 1) {
        return v;
    } else {
        static_assert(is_swappable_integer<T>,
            "The type is not supported for byte order conversions.");

        if constexpr (from != to) {
            return byteswap(v);
        } else {
            return v;
        }
    }
}

/// Returns `v` (in host order) converted to big endian byte order.
template<typename T>
T host_to_be(T v) {
    return convert_byte_order<host_byte_order, ByteOrder::BigEndian>(v);
}

/// Converts the big endian integer `v` to host order.
template<typename T>
T be_to_host(T v) {
    return convert_byte_order<ByteOrder::BigEndian, host_byte_order>(v);
}

} // namespace hammer

#endif // HAMMER_CORE_BYTE_ORDER_HPP
