#ifndef TIRO_COMPILER_SYNTAX_BUILD_SYNTAX_TREE_HPP
#define TIRO_COMPILER_SYNTAX_BUILD_SYNTAX_TREE_HPP

#include "common/adt/span.hpp"
#include "compiler/syntax/fwd.hpp"
#include "compiler/syntax/syntax_tree.hpp"

namespace tiro {

/// Constructs a concrete syntax tree from the given span of parser events.
/// Note that the span is modified as a side effect.
SyntaxTree build_syntax_tree(std::string_view source, Span<ParserEvent> events);

} // namespace tiro

#endif // TIRO_COMPILER_SYNTAX_BUILD_SYNTAX_TREE_HPP
