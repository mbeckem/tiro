#ifndef TIRO_COMPILER_PARSER_OPERATORS_HPP
#define TIRO_COMPILER_PARSER_OPERATORS_HPP

#include "common/format.hpp"
#include "compiler/ast/expr.hpp"
#include "compiler/parser/token.hpp"

namespace tiro {

// The common precedence for all unary operators.
extern const int unary_precedence;

// Returns the operator precedence for the given token type,
// when treated as an infix operator. Returns -1 if this is not
// an infix operator.
int infix_operator_precedence(TokenType t);

// Returns true iff the given binary operator is right associative.
bool operator_is_right_associative(BinaryOperator op);

// Attempts to parse the given token type as a unary operator.
std::optional<UnaryOperator> to_unary_operator(TokenType t);

// Attempts to parse the given token type as a binary operator.
std::optional<BinaryOperator> to_binary_operator(TokenType t);

} // namespace tiro

#endif // TIRO_COMPILER_PARSER_OPERATORS_HPP
