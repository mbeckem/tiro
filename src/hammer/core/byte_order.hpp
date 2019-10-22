#ifndef HAMMER_CORE_BYTE_ORDER_HPP
#define HAMMER_CORE_BYTE_ORDER_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/span.hpp"

namespace hammer {

/**
 * The possible values for the order of bytes within the binary representation of an integer. 
 */
enum class ByteOrder {
    BigEndian,
    LittleEndian,
};

// FIXME this is gcc specific
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
inline constexpr byte_order host_byte_order = ByteOrder::BigEndian;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
inline constexpr ByteOrder host_byte_order = ByteOrder::LittleEndian;
#endif

constexpr u16 byteswap(u16 v) {
    return __builtin_bswap16(v);
}

constexpr u32 byteswap(u32 v) {
    return __builtin_bswap32(v);
}

constexpr u64 byteswap(u64 v) {
    return __builtin_bswap64(v);
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
