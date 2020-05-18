#ifndef TIRO_CORE_ENUM_FLAGS_HPP
#define TIRO_CORE_ENUM_FLAGS_HPP

#include "tiro/core/defs.hpp"

#include <type_traits>

namespace tiro {

#define TIRO_INTERNAL_ENUM_BITWISE_1(EnumType, op)                    \
    constexpr EnumType operator op(EnumType value) {                  \
        return static_cast<EnumType>(                                 \
            op static_cast<std::underlying_type_t<EnumType>>(value)); \
    }

#define TIRO_INTERNAL_ENUM_BITWISE_2(EnumType, op)                      \
    constexpr EnumType operator op(EnumType lhs, EnumType rhs) {        \
        return static_cast<EnumType>(                                   \
            static_cast<std::underlying_type_t<EnumType>>(lhs)          \
                op static_cast<std::underlying_type_t<EnumType>>(rhs)); \
    }                                                                   \
                                                                        \
    constexpr EnumType& operator op##=(EnumType& lhs, EnumType rhs) {   \
        return lhs = (lhs op rhs);                                      \
    }

/// Defines bitwise operators for scoped enums that are being used as flags.
#define TIRO_DEFINE_ENUM_FLAGS(EnumType)      \
    TIRO_INTERNAL_ENUM_BITWISE_1(EnumType, ~) \
    TIRO_INTERNAL_ENUM_BITWISE_2(EnumType, |) \
    TIRO_INTERNAL_ENUM_BITWISE_2(EnumType, &) \
    TIRO_INTERNAL_ENUM_BITWISE_2(EnumType, ^)

} // namespace tiro

#endif // TIRO_CORE_ENUM_FLAGS_HPP
