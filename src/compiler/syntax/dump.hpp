#ifndef TIRO_COMPILER_SYNTAX_DUMP_HPP
#define TIRO_COMPILER_SYNTAX_DUMP_HPP

#include "compiler/fwd.hpp"
#include "compiler/syntax/fwd.hpp"

#include <string>

namespace tiro {

/// Output the tree as formatted json.
/// The map is used to transform raw byte offsets into line/column positions.
std::string dump(const SyntaxTree& tree, const SourceMap& map);

} // namespace tiro

#endif // TIRO_COMPILER_SYNTAX_DUMP_HPP
