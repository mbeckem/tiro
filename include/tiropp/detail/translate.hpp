#ifndef TIROPP_DETAIL_TRANSLATE_HPP_INCLUDED
#define TIROPP_DETAIL_TRANSLATE_HPP_INCLUDED

#include "tiro/def.h"

#include <string_view>

namespace tiro::detail {

inline std::string_view from_raw(const tiro_string_t& str) {
    return std::string_view(str.data, str.length);
}

inline tiro_string_t to_raw(const std::string_view& str) {
    return {str.data(), str.size()};
}

} // namespace tiro::detail

#endif // TIROPP_DETAIL_TRANSLATE_HPP_INCLUDED
