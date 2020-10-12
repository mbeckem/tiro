#ifndef TIRO_COMMON_TEXT_STRING_UTILS_HPP
#define TIRO_COMMON_TEXT_STRING_UTILS_HPP

#include "common/defs.hpp"

#include <string>

namespace tiro {

/// Escapes a string by escaping non-printable characters and control characters in it.
std::string escape_string(std::string_view input);

} // namespace tiro

#endif // TIRO_COMMON_TEXT_STRING_UTILS_HPP
