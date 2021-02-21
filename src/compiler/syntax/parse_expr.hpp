#ifndef TIRO_COMPILER_SYNTAX_PARSE_EXPR_HPP
#define TIRO_COMPILER_SYNTAX_PARSE_EXPR_HPP

#include "compiler/syntax/fwd.hpp"

#include <optional>

namespace tiro::next {

/// Tokens that may start an expression.
extern const TokenSet EXPR_FIRST;

std::optional<CompletedMarker> parse_expr(Parser& p, const TokenSet& recovery);

void parse_block_expr(Parser& p, const TokenSet& recovery);

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_PARSE_EXPR_HPP
