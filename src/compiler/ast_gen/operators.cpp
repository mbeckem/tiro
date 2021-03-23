#include "compiler/ast_gen/operators.hpp"

#include "compiler/ast/expr.hpp"
#include "compiler/syntax/token.hpp"

namespace tiro {

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
