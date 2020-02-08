#ifndef TIRO_COMPILER_UTILS
#define TIRO_COMPILER_UTILS

#include "tiro/core/defs.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace tiro::compiler {

/// Escapes a string by escaping non-printable characters and control characters in it.
std::string escape_string(std::string_view input);

struct StringTree {
    std::string line;
    std::vector<StringTree> children;
};

/// Formats a recursive tree structure as a string.
/// Not very efficient approach but this is only for console I/O and debugging.
std::string format_tree(const StringTree& tree);

} // namespace tiro::compiler

#endif // TIRO_COMPILER_UTILS
