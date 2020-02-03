#ifndef TIRO_COMPILER_UTILS
#define TIRO_COMPILER_UTILS

#include "tiro/core/defs.hpp"

#include <string>
#include <string_view>

namespace tiro::compiler {

/// Escapes a string by escaping non-printable characters and control characters in it.
std::string escape_string(std::string_view input);

} // namespace tiro::compiler

#endif // TIRO_COMPILER_UTILS
