#ifndef TIRO_AST_OPERATORS_HPP
#define TIRO_AST_OPERATORS_HPP

#include "tiro/ast/token.hpp"
#include "tiro/core/format.hpp"

namespace tiro {

/// The operator used in a unary operation.
enum class UnaryOperator : u8 {
    // Arithmetic
    Plus,
    Minus,

    // Binary
    BitwiseNot,

    // Boolean
    LogicalNot
};

std::string_view to_string(UnaryOperator op);

/// The operator used in a binary operation.
enum class BinaryOperator : u8 {
    // Arithmetic
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulus,
    Power,

    // Binary
    LeftShift,
    RightShift,
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,

    // Boolean
    Less,
    LessEquals,
    Greater,
    GreaterEquals,
    Equals,
    NotEquals,
    LogicalAnd,
    LogicalOr,

    // Assigments
    // TODO: Factor these out into a new node type. They are too different.
    Assign,
    AssignPlus,
    AssignMinus,
    AssignMultiply,
    AssignDivide,
    AssignModulus,
    AssignPower
};

std::string_view to_string(BinaryOperator op);

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

TIRO_ENABLE_FREE_TO_STRING(tiro::UnaryOperator)
TIRO_ENABLE_FREE_TO_STRING(tiro::BinaryOperator)

#endif // TIRO_AST_OPERATORS_HPP
