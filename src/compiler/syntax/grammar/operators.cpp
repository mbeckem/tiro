#include "compiler/syntax/grammar/operators.hpp"

#include "compiler/syntax/token.hpp"

namespace tiro::next {

const int unary_precedence = 13;

std::optional<InfixOperator> infix_operator_precedence(TokenType t) {
    switch (t) {
    case TokenType::Equals:
    case TokenType::PlusEquals:
    case TokenType::MinusEquals:
    case TokenType::StarEquals:
    case TokenType::StarStarEquals:
    case TokenType::SlashEquals:
    case TokenType::PercentEquals:
        return InfixOperator{0, true};

    case TokenType::LogicalOr:
        return InfixOperator{1, false};

    case TokenType::LogicalAnd:
        return InfixOperator{2, false};

    case TokenType::QuestionQuestion:
        return InfixOperator{3, false};

    case TokenType::BitwiseOr:
        return InfixOperator{4, false};

    case TokenType::BitwiseXor:
        return InfixOperator{5, false};

    case TokenType::BitwiseAnd:
        return InfixOperator{6, false};

    case TokenType::EqualsEquals:
    case TokenType::NotEquals:
        return InfixOperator{7, false};

    case TokenType::Less:
    case TokenType::LessEquals:
    case TokenType::Greater:
    case TokenType::GreaterEquals:
        return InfixOperator{8, false};

    case TokenType::LeftShift:
    case TokenType::RightShift:
        return InfixOperator{9, false};

    case TokenType::Plus:
    case TokenType::Minus:
        return InfixOperator{10, false};

    case TokenType::Star:    // Multiply
    case TokenType::Slash:   // Divide
    case TokenType::Percent: // Modulus
        return InfixOperator{11, false};

    case TokenType::StarStar: // Power
        return InfixOperator{12, true};

        // UNARY OPERATORS == 13

    case TokenType::LeftParen:           // Function call
    case TokenType::LeftBracket:         // Element access
    case TokenType::Dot:                 // Member access
    case TokenType::QuestionLeftParen:   // Optional function call
    case TokenType::QuestionLeftBracket: // Optional element access
    case TokenType::QuestionDot:         // Optional member access
        return InfixOperator{14, false};

    default:
        return {};
    }
}

} // namespace tiro::next
