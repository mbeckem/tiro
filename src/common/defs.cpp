#include "common/defs.hpp"

#include <climits>
#include <type_traits>

namespace tiro {

/// Guards against weird platforms.
static_assert(CHAR_BIT == 8, "Bytes with a size other than 8 bits are not supported.");
static_assert(std::is_same_v<char, u8> || std::is_same_v<unsigned char, u8>,
    "uint8_t must be either char or unsigned char.");
static_assert(std::is_same_v<char, i8> || std::is_same_v<signed char, i8>,
    "int8_t must be either char or signed char.");
static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

} // namespace tiro
