#ifndef TIRO_COMPILER_SYNTAX_GRAMMAR_EXPR_HPP
#define TIRO_COMPILER_SYNTAX_GRAMMAR_EXPR_HPP

#include "compiler/syntax/fwd.hpp"

#include <optional>

namespace tiro::next {

/// Tokens that may start an expression.
extern const TokenSet EXPR_FIRST;

/// Parses all complete expressions.
std::optional<CompletedMarker> parse_expr(Parser& p, const TokenSet& recovery);

/// Parses all expressions EXCEPT for those that would introduce ambiguities before the start of a block.
/// For example, in `if map { ... }` we might have either a map{...} or an if statement with the condition `map`.
/// This function would eat only the map identifier, leaving the `{...}` for the if statements block expression.
std::optional<CompletedMarker> parse_expr_no_block(Parser& p, const TokenSet& recovery);

void parse_block_expr(Parser& p, const TokenSet& recovery);

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_GRAMMAR_EXPR_HPP
