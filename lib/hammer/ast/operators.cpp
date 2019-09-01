#include "hammer/ast/operators.hpp"

#include "hammer/core/defs.hpp"

namespace hammer::ast {

const char* to_string(UnaryOperator op) {
#define HAMMER_CASE(op)     \
    case UnaryOperator::op: \
        return #op;

    switch (op) {
        HAMMER_CASE(Plus)
        HAMMER_CASE(Minus)
        HAMMER_CASE(BitwiseNot)
        HAMMER_CASE(LogicalNot)
    }

#undef HAMMER_CASE

    HAMMER_UNREACHABLE("Invalid unary operation.");
}

const char* to_string(BinaryOperator op) {
#define HAMMER_CASE(op)      \
    case BinaryOperator::op: \
        return #op;

    switch (op) {
        HAMMER_CASE(Plus)
        HAMMER_CASE(Minus)
        HAMMER_CASE(Multiply)
        HAMMER_CASE(Divide)
        HAMMER_CASE(Modulus)
        HAMMER_CASE(Power)
        HAMMER_CASE(LeftShift)
        HAMMER_CASE(RightShift)
        HAMMER_CASE(BitwiseOr)
        HAMMER_CASE(BitwiseXor)
        HAMMER_CASE(BitwiseAnd)

        HAMMER_CASE(Less)
        HAMMER_CASE(LessEq)
        HAMMER_CASE(Greater)
        HAMMER_CASE(GreaterEq)
        HAMMER_CASE(Equals)
        HAMMER_CASE(NotEquals)
        HAMMER_CASE(LogicalAnd)
        HAMMER_CASE(LogicalOr)

        HAMMER_CASE(Assign)
    }

#undef HAMMER_CASE

    HAMMER_UNREACHABLE("Invalid binary operation.");
}

int operator_precedence(BinaryOperator op) {
    switch (op) {
    case BinaryOperator::Assign:
        return 0;

    case BinaryOperator::LogicalOr:
        return 1;

    case BinaryOperator::LogicalAnd:
        return 2;

    case BinaryOperator::BitwiseOr:
        return 3;

    case BinaryOperator::BitwiseXor:
        return 4;

    case BinaryOperator::BitwiseAnd:
        return 5;

    case BinaryOperator::Equals:
    case BinaryOperator::NotEquals:
        return 6;

    case BinaryOperator::Less:
    case BinaryOperator::LessEq:
    case BinaryOperator::Greater:
    case BinaryOperator::GreaterEq:
        return 7;

    case BinaryOperator::LeftShift:
    case BinaryOperator::RightShift:
        return 8;

    case BinaryOperator::Plus:
    case BinaryOperator::Minus:
        return 9;

    case BinaryOperator::Multiply:
    case BinaryOperator::Divide:
    case BinaryOperator::Modulus:
        return 10;

    case BinaryOperator::Power:
        return 11;
    }

    HAMMER_UNREACHABLE("Invalid binary operation type.");
}

bool operator_is_right_associative(BinaryOperator op) {
    switch (op) {
    case BinaryOperator::Assign:
    case BinaryOperator::Power:
        return true;
    default:
        return false;
    }
}

} // namespace hammer::ast
