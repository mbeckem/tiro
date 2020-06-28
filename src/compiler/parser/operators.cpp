#include "compiler/parser/operators.hpp"

namespace tiro {

const int unary_precedence = 13;

int infix_operator_precedence(TokenType t) {
    switch (t) {
    // Assigment
    case TokenType::Equals:
    case TokenType::PlusEquals:
    case TokenType::MinusEquals:
    case TokenType::StarEquals:
    case TokenType::StarStarEquals:
    case TokenType::SlashEquals:
    case TokenType::PercentEquals:
        return 0;

    case TokenType::LogicalOr:
        return 1;

    case TokenType::LogicalAnd:
        return 2;

    case TokenType::QuestionQuestion:
        return 3;

    case TokenType::BitwiseOr:
        return 4;

    case TokenType::BitwiseXor:
        return 5;

    case TokenType::BitwiseAnd:
        return 6;

    case TokenType::EqualsEquals:
    case TokenType::NotEquals:
        return 7;

    case TokenType::Less:
    case TokenType::LessEquals:
    case TokenType::Greater:
    case TokenType::GreaterEquals:
        return 8;

    case TokenType::LeftShift:
    case TokenType::RightShift:
        return 9;

    case TokenType::Plus:
    case TokenType::Minus:
        return 10;

    case TokenType::Star:    // Multiply
    case TokenType::Slash:   // Divide
    case TokenType::Percent: // Modulus
        return 11;

    case TokenType::StarStar: // Power
        return 12;

        // UNARY OPERATORS == 13

    case TokenType::LeftParen:           // Function call
    case TokenType::LeftBracket:         // Element acess
    case TokenType::Dot:                 // Member access
    case TokenType::QuestionLeftParen:   // Optional function call
    case TokenType::QuestionLeftBracket: // Optional element access
    case TokenType::QuestionDot:         // Optional member access
        return 14;

    default:
        return -1;
    }
}

bool operator_is_right_associative(BinaryOperator op) {
    switch (op) {
    case BinaryOperator::Assign:
    case BinaryOperator::AssignPlus:
    case BinaryOperator::AssignMinus:
    case BinaryOperator::AssignMultiply:
    case BinaryOperator::AssignPower:
    case BinaryOperator::AssignDivide:
    case BinaryOperator::AssignModulus:
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
#define TIRO_MAP_TOKEN(token, op) \
    case TokenType::token:        \
        return BinaryOperator::op;

    switch (t) {
        TIRO_MAP_TOKEN(Plus, Plus)
        TIRO_MAP_TOKEN(Minus, Minus)
        TIRO_MAP_TOKEN(Star, Multiply)
        TIRO_MAP_TOKEN(Slash, Divide)
        TIRO_MAP_TOKEN(Percent, Modulus)
        TIRO_MAP_TOKEN(StarStar, Power)
        TIRO_MAP_TOKEN(LeftShift, LeftShift)
        TIRO_MAP_TOKEN(RightShift, RightShift)

        TIRO_MAP_TOKEN(BitwiseAnd, BitwiseAnd)
        TIRO_MAP_TOKEN(BitwiseOr, BitwiseOr)
        TIRO_MAP_TOKEN(BitwiseXor, BitwiseXor)

        TIRO_MAP_TOKEN(Less, Less)
        TIRO_MAP_TOKEN(LessEquals, LessEquals)
        TIRO_MAP_TOKEN(Greater, Greater)
        TIRO_MAP_TOKEN(GreaterEquals, GreaterEquals)
        TIRO_MAP_TOKEN(EqualsEquals, Equals)
        TIRO_MAP_TOKEN(NotEquals, NotEquals)
        TIRO_MAP_TOKEN(LogicalAnd, LogicalAnd)
        TIRO_MAP_TOKEN(LogicalOr, LogicalOr)
        TIRO_MAP_TOKEN(QuestionQuestion, NullCoalesce)

        TIRO_MAP_TOKEN(Equals, Assign)
        TIRO_MAP_TOKEN(PlusEquals, AssignPlus)
        TIRO_MAP_TOKEN(MinusEquals, AssignMinus)
        TIRO_MAP_TOKEN(StarEquals, AssignMultiply)
        TIRO_MAP_TOKEN(StarStarEquals, AssignPower)
        TIRO_MAP_TOKEN(SlashEquals, AssignDivide)
        TIRO_MAP_TOKEN(PercentEquals, AssignModulus)

    default:
        return {};
    }

#undef TIRO_MAP_TOKEN
}

} // namespace tiro
