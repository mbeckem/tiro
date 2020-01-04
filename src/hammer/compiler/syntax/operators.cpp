#include "hammer/compiler/syntax/operators.hpp"

namespace hammer::compiler {

const int unary_precedence = 12;

int infix_operator_precedence(TokenType t) {
    switch (t) {
    // Assigment
    case TokenType::Equals:
        return 0;

    case TokenType::LogicalOr:
        return 1;

    case TokenType::LogicalAnd:
        return 2;

    case TokenType::BitwiseOr:
        return 3;

    case TokenType::BitwiseXor:
        return 4;

    case TokenType::BitwiseAnd:
        return 5;

    // TODO Reconsider precendence of equality: should it be lower than Bitwise xor/or/and?
    case TokenType::EqualsEquals:
    case TokenType::NotEquals:
        return 6;

    case TokenType::Less:
    case TokenType::LessEquals:
    case TokenType::Greater:
    case TokenType::GreaterEquals:
        return 7;

    case TokenType::LeftShift:
    case TokenType::RightShift:
        return 8;

    case TokenType::Plus:
    case TokenType::Minus:
        return 9;

    case TokenType::Star:    // Multiply
    case TokenType::Slash:   // Divide
    case TokenType::Percent: // Modulus
        return 10;

    case TokenType::StarStar: // Power
        return 11;

        // UNARY OPERATORS == 12

    case TokenType::LeftParen:   // Function call
    case TokenType::LeftBracket: // Array
    case TokenType::Dot:         // Member access
        return 13;

    default:
        return -1;
    }
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

std::optional<UnaryOperator> to_unary_operator(TokenType t) {
    switch (t) {
    case TokenType::Plus:
        return UnaryOperator::Plus;
    case TokenType::Minus:
        return UnaryOperator::Minus;
    case TokenType::LogicalNot:
        return UnaryOperator::LogicalNot;
    case TokenType::BitwiseNot:
        return UnaryOperator::BitwiseNot;
    default:
        return {};
    }
}

std::optional<BinaryOperator> to_binary_operator(TokenType t) {
#define HAMMER_MAP_TOKEN(token, op) \
    case TokenType::token:          \
        return BinaryOperator::op;

    switch (t) {
        HAMMER_MAP_TOKEN(Plus, Plus)
        HAMMER_MAP_TOKEN(Minus, Minus)
        HAMMER_MAP_TOKEN(Star, Multiply)
        HAMMER_MAP_TOKEN(Slash, Divide)
        HAMMER_MAP_TOKEN(Percent, Modulus)
        HAMMER_MAP_TOKEN(StarStar, Power)
        HAMMER_MAP_TOKEN(LeftShift, LeftShift)
        HAMMER_MAP_TOKEN(RightShift, RightShift)

        HAMMER_MAP_TOKEN(BitwiseAnd, BitwiseAnd)
        HAMMER_MAP_TOKEN(BitwiseOr, BitwiseOr)
        HAMMER_MAP_TOKEN(BitwiseXor, BitwiseXor)

        HAMMER_MAP_TOKEN(Less, Less)
        HAMMER_MAP_TOKEN(LessEquals, LessEquals)
        HAMMER_MAP_TOKEN(Greater, Greater)
        HAMMER_MAP_TOKEN(GreaterEquals, GreaterEquals)
        HAMMER_MAP_TOKEN(EqualsEquals, Equals)
        HAMMER_MAP_TOKEN(NotEquals, NotEquals)
        HAMMER_MAP_TOKEN(LogicalAnd, LogicalAnd)
        HAMMER_MAP_TOKEN(LogicalOr, LogicalOr)

        HAMMER_MAP_TOKEN(Equals, Assign)

    default:
        return {};
    }

#undef HAMMER_MAP_TOKEN
}

} // namespace hammer::compiler
