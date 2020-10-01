#ifndef TIRO_COMPILER_UTILS_HPP
#define TIRO_COMPILER_UTILS_HPP

#include "common/defs.hpp"

#include <string>
#include <vector>

namespace tiro {

struct StringTree {
    std::string line;
    std::vector<StringTree> children;
};

/// Formats a recursive tree structure as a string.
/// Not very efficient approach but this is only for console I/O and debugging.
std::string format_tree(const StringTree& tree);

} // namespace tiro

#endif // TIRO_COMPILER_UTILS_HPP
