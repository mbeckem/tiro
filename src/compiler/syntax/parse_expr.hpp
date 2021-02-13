#ifndef TIRO_COMPILER_SYNTAX_PARSE_EXPR_HPP
#define TIRO_COMPILER_SYNTAX_PARSE_EXPR_HPP

#include "compiler/syntax/parser.hpp"

#include <optional>

namespace tiro::next {

std::optional<Parser::CompletedMarker> parse_expr(Parser& p, const TokenSet& recovery);

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_PARSE_EXPR_HPP
