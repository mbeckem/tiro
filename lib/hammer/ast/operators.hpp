#ifndef HAMMER_AST_OPERATORS_HPP
#define HAMMER_AST_OPERATORS_HPP

namespace hammer::ast {

enum class UnaryOperator : int {
    // Arithmetic
    plus,
    minus,

    // Binary
    bitwise_not,

    // Boolean
    logical_not
};

const char* to_string(UnaryOperator op);

enum class BinaryOperator : int {
    // Arithmetic
    plus,
    minus,
    multiply,
    divide,
    modulus,
    power,

    // Binary
    left_shift,
    right_shift,
    bitwise_and,
    bitwise_or,
    bitwise_xor,

    // Boolean
    less,
    less_eq,
    greater,
    greater_eq,
    equals,
    not_equals,
    logical_and,
    logical_or,

    // Assigments
    assign,
};

const char* to_string(BinaryOperator op);

// Returns the precedence of the given binary operator.
int operator_precedence(BinaryOperator op);

// Returns true if the given operator is right-associative (e.g. 2^3^4 is 2^(3^4))
bool operator_is_right_associative(BinaryOperator op);

} // namespace hammer::ast

#endif // HAMMER_AST_OPERATORS_HPP
