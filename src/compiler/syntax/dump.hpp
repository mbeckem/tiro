#ifndef TIRO_COMPILER_SYNTAX_DUMP_HPP
#define TIRO_COMPILER_SYNTAX_DUMP_HPP

#include "compiler/syntax/fwd.hpp"

#include <string>

namespace tiro {

/// Output the tree as formatted json.
std::string dump(const SyntaxTree& tree);

} // namespace tiro

#endif // TIRO_COMPILER_SYNTAX_DUMP_HPP
