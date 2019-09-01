#ifndef HAMMER_AST_OPERATORS_HPP
#define HAMMER_AST_OPERATORS_HPP

namespace hammer::ast {

enum class UnaryOperator : int {
    // Arithmetic
    Plus,
    Minus,

    // Binary
    BitwiseNot,

    // Boolean
    LogicalNot
};

const char* to_string(UnaryOperator op);

enum class BinaryOperator : int {
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
    LessEq,
    Greater,
    GreaterEq,
    Equals,
    NotEquals,
    LogicalAnd,
    LogicalOr,

    // Assigments
    Assign,
};

const char* to_string(BinaryOperator op);

// Returns the precedence of the given binary operator.
int operator_precedence(BinaryOperator op);

// Returns true if the given operator is right-associative (e.g. 2^3^4 is 2^(3^4))
bool operator_is_right_associative(BinaryOperator op);

} // namespace hammer::ast

#endif // HAMMER_AST_OPERATORS_HPP
