#include "vm/hash.hpp"

#include "common/type_traits.hpp"

#include <cmath>
#include <cstdint>

namespace tiro::vm {

[[maybe_unused]] static u64 fnv1a_64(Span<const byte> data) {
    static constexpr u64 magic_prime = UINT64_C(0x00000100000001b3);

    u64 hash = UINT64_C(0xcbf29ce484222325);
    for (byte b : data) {
        hash = hash ^ b;
        hash = hash * magic_prime;
    }
    return hash;
}

[[maybe_unused]] static u32 fnv1a_32(Span<const byte> data) {
    static constexpr u32 magic_prime = UINT32_C(0x01000193);

    u32 hash = UINT32_C(0x811c9dc5);
    for (byte b : data) {
        hash = hash ^ b;
        hash = hash * magic_prime;
    }
    return hash;
}

static size_t fnv1a(Span<const byte> data) {
    static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8);

    if constexpr (sizeof(size_t) == 4) {
        return static_cast<size_t>(fnv1a_32(data));
    } else if constexpr (sizeof(size_t) == 8) {
        return static_cast<size_t>(fnv1a_64(data));
    } else {
        TIRO_UNREACHABLE(
            "Unsupported architecture (machine must be "
            "32 or 64 bit).");
    }
}

size_t byte_hash(Span<const byte> data) {
    return fnv1a(data);
}

size_t integer_hash(u64 data) {
    return fnv1a(raw_span(data));
}

static_assert(std::numeric_limits<f64>::has_quiet_NaN,
    "Platform lacks quiet nan, which is currently required.");

static constexpr f64 canonic_nan = std::numeric_limits<f64>::quiet_NaN();

size_t float_hash(f64 data) {
    // Ensure that both -0.0 and 0.0 hash to the same value.
    if (data == 0)
        return fnv1a(raw_span<u64>(0));

    // All nans hash to the same value to enable their usage in hash tables.
    if (std::isnan(data))
        return fnv1a(raw_span(canonic_nan));

    return fnv1a(raw_span(data));
}

} // namespace tiro::vm
