#include "hammer/ast/operators.hpp"

#include "hammer/core/defs.hpp"

namespace hammer::ast {

const char* to_string(UnaryOperator op) {
#define HAMMER_CASE(op)     \
    case UnaryOperator::op: \
        return #op;

    switch (op) {
        HAMMER_CASE(plus)
        HAMMER_CASE(minus)
        HAMMER_CASE(bitwise_not)
        HAMMER_CASE(logical_not)
    }

#undef HAMMER_CASE

    HAMMER_UNREACHABLE("Invalid unary operation.");
}

const char* to_string(BinaryOperator op) {
#define HAMMER_CASE(op)      \
    case BinaryOperator::op: \
        return #op;

    switch (op) {
        HAMMER_CASE(plus)
        HAMMER_CASE(minus)
        HAMMER_CASE(multiply)
        HAMMER_CASE(divide)
        HAMMER_CASE(modulus)
        HAMMER_CASE(power)
        HAMMER_CASE(left_shift)
        HAMMER_CASE(right_shift)
        HAMMER_CASE(bitwise_or)
        HAMMER_CASE(bitwise_xor)
        HAMMER_CASE(bitwise_and)

        HAMMER_CASE(less)
        HAMMER_CASE(less_eq)
        HAMMER_CASE(greater)
        HAMMER_CASE(greater_eq)
        HAMMER_CASE(equals)
        HAMMER_CASE(not_equals)
        HAMMER_CASE(logical_and)
        HAMMER_CASE(logical_or)

        HAMMER_CASE(assign)
    }

#undef HAMMER_CASE

    HAMMER_UNREACHABLE("Invalid binary operation.");
}

int operator_precedence(BinaryOperator op) {
    switch (op) {
    case BinaryOperator::assign:
        return 0;

    case BinaryOperator::logical_or:
        return 1;

    case BinaryOperator::logical_and:
        return 2;

    case BinaryOperator::bitwise_or:
        return 3;

    case BinaryOperator::bitwise_xor:
        return 4;

    case BinaryOperator::bitwise_and:
        return 5;

    case BinaryOperator::equals:
    case BinaryOperator::not_equals:
        return 6;

    case BinaryOperator::less:
    case BinaryOperator::less_eq:
    case BinaryOperator::greater:
    case BinaryOperator::greater_eq:
        return 7;

    case BinaryOperator::left_shift:
    case BinaryOperator::right_shift:
        return 8;

    case BinaryOperator::plus:
    case BinaryOperator::minus:
        return 9;

    case BinaryOperator::multiply:
    case BinaryOperator::divide:
    case BinaryOperator::modulus:
        return 10;

    case BinaryOperator::power:
        return 11;
    }

    HAMMER_UNREACHABLE("Invalid binary operation type.");
}

bool operator_is_right_associative(BinaryOperator op) {
    switch (op) {
    case BinaryOperator::assign:
    case BinaryOperator::power:
        return true;
    default:
        return false;
    }
}

} // namespace hammer::ast
