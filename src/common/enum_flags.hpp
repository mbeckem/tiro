#ifndef TIRO_COMMON_ENUM_FLAGS_HPP
#define TIRO_COMMON_ENUM_FLAGS_HPP

#include "common/defs.hpp"

#include <type_traits>

namespace tiro {

/// Implements a set of bitflags. The given enum type should be defined like this:
///
///     enum MyFlags {
///         A = 1 << 0,
///         B = 1 << 1,
///     };
///
/// Internally, bitwise AND and OR are used to determine whether a flag is set or not.
template<typename EnumType>
class Flags {
public:
    using UnderlyingType = std::underlying_type_t<EnumType>;

    constexpr Flags()
        : value_(0) {}

    constexpr Flags(EnumType value)
        : value_(as_raw(value)) {}

    /// Returns true if all flag values in `flags` are set.
    constexpr bool test(Flags<EnumType> flags) const noexcept {
        return (value_ & flags.raw()) == flags.raw();
    }

    /// Sets all flag values in `flags` to `true`.
    constexpr void set(Flags<EnumType> flags) noexcept { set(flags, true); }

    /// Sets all flag values in `flags` to the specified value.
    constexpr void set(Flags<EnumType> flags, bool value) noexcept {
        if (value) {
            value_ |= flags.raw();
        } else {
            value_ &= ~flags.raw();
        }
    }

    /// Sets all flag values in `flags` to false.
    constexpr void clear(Flags<EnumType> flags) noexcept { set(flags, false); }

    /// Unsets all flag values.
    constexpr void clear() noexcept { value_ = 0; }

    /// Returns the raw underlying representation.
    constexpr UnderlyingType raw() const noexcept { return value_; }

private:
    static constexpr UnderlyingType as_raw(EnumType value) noexcept {
        return static_cast<UnderlyingType>(value);
    }

private:
    UnderlyingType value_;
};

} // namespace tiro

#endif // TIRO_COMMON_ENUM_FLAGS_HPP
