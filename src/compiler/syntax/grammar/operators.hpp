#ifndef TIRO_COMPILER_SYNTAX_GRAMMAR_OPERATORS_HPP
#define TIRO_COMPILER_SYNTAX_GRAMMAR_OPERATORS_HPP

#include "compiler/syntax/fwd.hpp"

#include <optional>

namespace tiro::next {

struct InfixOperator {
    // Higher precedence value -> stronger binding power.
    int precedence;

    // True if the operator is right associative.
    bool right_assoc;
};

// The common precedence value for all unary operators.
extern const int unary_precedence;

// Returns the operator precedence and the associativity for the given token type.
// Returns an empty optional if the token is not an infix operator.
std::optional<InfixOperator> infix_operator_precedence(TokenType t);

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_GRAMMAR_OPERATORS_HPP
