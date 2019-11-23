#include "hammer/compiler/token.hpp"

#include "hammer/core/defs.hpp"

namespace hammer {

std::string_view to_token_name(TokenType tok) {
    switch (tok) {
#define HAMMER_CASE(x)   \
    case (TokenType::x): \
        return #x;

        HAMMER_CASE(InvalidToken)
        HAMMER_CASE(Eof)
        HAMMER_CASE(Comment)

        HAMMER_CASE(Identifier)
        HAMMER_CASE(SymbolLiteral)
        HAMMER_CASE(StringLiteral)
        HAMMER_CASE(FloatLiteral)
        HAMMER_CASE(IntegerLiteral)

        HAMMER_CASE(KwFunc)
        HAMMER_CASE(KwVar)
        HAMMER_CASE(KwConst)
        HAMMER_CASE(KwIf)
        HAMMER_CASE(KwElse)
        HAMMER_CASE(KwWhile)
        HAMMER_CASE(KwFor)
        HAMMER_CASE(KwContinue)
        HAMMER_CASE(KwBreak)
        HAMMER_CASE(KwReturn)
        HAMMER_CASE(KwSwitch)
        HAMMER_CASE(KwClass)
        HAMMER_CASE(KwStruct)
        HAMMER_CASE(KwProtocol)
        HAMMER_CASE(KwAssert)
        HAMMER_CASE(KwTrue)
        HAMMER_CASE(KwFalse)
        HAMMER_CASE(KwNull)
        HAMMER_CASE(KwImport)
        HAMMER_CASE(KwExport)
        HAMMER_CASE(KwPackage)
        HAMMER_CASE(KwMap)
        HAMMER_CASE(KwSet)

        HAMMER_CASE(KwYield)
        HAMMER_CASE(KwAsync)
        HAMMER_CASE(KwAwait)
        HAMMER_CASE(KwThrow)
        HAMMER_CASE(KwTry)
        HAMMER_CASE(KwCatch)
        HAMMER_CASE(KwScope)

        HAMMER_CASE(LeftParen)
        HAMMER_CASE(RightParen)
        HAMMER_CASE(LeftBracket)
        HAMMER_CASE(RightBracket)
        HAMMER_CASE(LeftBrace)
        HAMMER_CASE(RightBrace)

        HAMMER_CASE(Dot)
        HAMMER_CASE(Comma)
        HAMMER_CASE(Colon)
        HAMMER_CASE(Semicolon)
        HAMMER_CASE(Question)
        HAMMER_CASE(Plus)
        HAMMER_CASE(Minus)
        HAMMER_CASE(Star)
        HAMMER_CASE(StarStar)
        HAMMER_CASE(Slash)
        HAMMER_CASE(Percent)
        HAMMER_CASE(PlusPlus)
        HAMMER_CASE(MinusMinus)
        HAMMER_CASE(BitwiseNot)
        HAMMER_CASE(BitwiseOr)
        HAMMER_CASE(BitwiseXor)
        HAMMER_CASE(BitwiseAnd)
        HAMMER_CASE(LeftShift)
        HAMMER_CASE(RightShift)
        HAMMER_CASE(LogicalNot)
        HAMMER_CASE(LogicalOr)
        HAMMER_CASE(LogicalAnd)
        HAMMER_CASE(Equals)
        HAMMER_CASE(EqualsEquals)
        HAMMER_CASE(NotEquals)
        HAMMER_CASE(Less)
        HAMMER_CASE(Greater)
        HAMMER_CASE(LessEquals)
        HAMMER_CASE(GreaterEquals)

#undef HAMMER_CASE
    }

    HAMMER_UNREACHABLE("Invalid token type");
}

std::string_view to_description(TokenType tok) {
    switch (tok) {
#define HAMMER_CASE(x, s) \
    case TokenType::x:    \
        return s;

#define HAMMER_CASE_Q(x, s) HAMMER_CASE(x, "'" s "'")

        HAMMER_CASE(InvalidToken, "<invalid_token>")
        HAMMER_CASE(Eof, "<end of file>")
        HAMMER_CASE(Comment, "<comment>")

        HAMMER_CASE(Identifier, "<identifier>")
        HAMMER_CASE(SymbolLiteral, "<symbol>")
        HAMMER_CASE(StringLiteral, "<string>")
        HAMMER_CASE(FloatLiteral, "<float>")
        HAMMER_CASE(IntegerLiteral, "<integer>")

        HAMMER_CASE_Q(KwFunc, "func")
        HAMMER_CASE_Q(KwVar, "var")
        HAMMER_CASE_Q(KwConst, "const")
        HAMMER_CASE_Q(KwIf, "if")
        HAMMER_CASE_Q(KwElse, "else")
        HAMMER_CASE_Q(KwWhile, "while")
        HAMMER_CASE_Q(KwFor, "for")
        HAMMER_CASE_Q(KwContinue, "continue")
        HAMMER_CASE_Q(KwBreak, "break")
        HAMMER_CASE_Q(KwReturn, "return")
        HAMMER_CASE_Q(KwSwitch, "switch")
        HAMMER_CASE_Q(KwClass, "class")
        HAMMER_CASE_Q(KwStruct, "struct")
        HAMMER_CASE_Q(KwProtocol, "protocol")
        HAMMER_CASE_Q(KwAssert, "assert")
        HAMMER_CASE_Q(KwTrue, "true")
        HAMMER_CASE_Q(KwFalse, "false")
        HAMMER_CASE_Q(KwNull, "null")
        HAMMER_CASE_Q(KwImport, "import")
        HAMMER_CASE_Q(KwExport, "export")
        HAMMER_CASE_Q(KwPackage, "package")
        HAMMER_CASE_Q(KwMap, "Map")
        HAMMER_CASE_Q(KwSet, "Set")

        HAMMER_CASE_Q(KwYield, "yield")
        HAMMER_CASE_Q(KwAsync, "async")
        HAMMER_CASE_Q(KwAwait, "await")
        HAMMER_CASE_Q(KwThrow, "throw")
        HAMMER_CASE_Q(KwTry, "try")
        HAMMER_CASE_Q(KwCatch, "catch")
        HAMMER_CASE_Q(KwScope, "scope")

        HAMMER_CASE_Q(LeftParen, "(")
        HAMMER_CASE_Q(RightParen, ")")
        HAMMER_CASE_Q(LeftBracket, "[")
        HAMMER_CASE_Q(RightBracket, "]")
        HAMMER_CASE_Q(LeftBrace, "{")
        HAMMER_CASE_Q(RightBrace, "}")

        HAMMER_CASE_Q(Dot, ".")
        HAMMER_CASE_Q(Comma, ",")
        HAMMER_CASE_Q(Colon, ":")
        HAMMER_CASE_Q(Semicolon, ";")
        HAMMER_CASE_Q(Question, "?")
        HAMMER_CASE_Q(Plus, "+")
        HAMMER_CASE_Q(Minus, "-")
        HAMMER_CASE_Q(Star, "*")
        HAMMER_CASE_Q(StarStar, "**")
        HAMMER_CASE_Q(Slash, "/")
        HAMMER_CASE_Q(Percent, "%")
        HAMMER_CASE_Q(PlusPlus, "++")
        HAMMER_CASE_Q(MinusMinus, "--")
        HAMMER_CASE_Q(BitwiseNot, "~")
        HAMMER_CASE_Q(BitwiseOr, "|")
        HAMMER_CASE_Q(BitwiseXor, "^")
        HAMMER_CASE_Q(BitwiseAnd, "&")
        HAMMER_CASE_Q(LeftShift, "<<")
        HAMMER_CASE_Q(RightShift, ">>")
        HAMMER_CASE_Q(LogicalNot, "!")
        HAMMER_CASE_Q(LogicalOr, "||")
        HAMMER_CASE_Q(LogicalAnd, "&&")
        HAMMER_CASE_Q(Equals, "=")
        HAMMER_CASE_Q(EqualsEquals, "==")
        HAMMER_CASE_Q(NotEquals, "!=")
        HAMMER_CASE_Q(Less, "<")
        HAMMER_CASE_Q(Greater, ">")
        HAMMER_CASE_Q(LessEquals, "<=")
        HAMMER_CASE_Q(GreaterEquals, ">=")

#undef HAMMER_CASE
#undef HAMMER_CASE_Q
    }

    HAMMER_UNREACHABLE("Invalid token type");
}

i64 Token::int_value() const {
    HAMMER_ASSERT(std::holds_alternative<i64>(value_),
        "Token does not contain an integer value.");
    return std::get<i64>(value_);
}

f64 Token::float_value() const {
    HAMMER_ASSERT(std::holds_alternative<f64>(value_),
        "Token does not contain a float value.");
    return std::get<f64>(value_);
}

InternedString Token::string_value() const {
    HAMMER_ASSERT(std::holds_alternative<InternedString>(value_),
        "Token does not contain a string value.");
    return std::get<InternedString>(value_);
}

std::string TokenTypes::to_string() const {
    fmt::memory_buffer buf;

    fmt::format_to(buf, "TokenTypes{{");
    {
        bool first = true;
        for (TokenType type : *this) {
            if (!first)
                fmt::format_to(buf, ", ");

            fmt::format_to(buf, "{}", hammer::to_token_name(type));
            first = false;
        }
    }
    fmt::format_to(buf, "}}");

    return fmt::to_string(buf);
}

} // namespace hammer
