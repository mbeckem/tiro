#ifndef TIRO_COMPILER_SYNTAX_PARSE_ITEM_HPP
#define TIRO_COMPILER_SYNTAX_PARSE_ITEM_HPP

#include "compiler/syntax/fwd.hpp"

namespace tiro::next {

/// Parses a top level item (imports, function declarations... ).
void parse_item(Parser& p, const TokenSet& recovery);

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_PARSE_ITEM_HPP
