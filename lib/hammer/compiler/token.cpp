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
        HAMMER_CASE(KwTrue)
        HAMMER_CASE(KwFalse)
        HAMMER_CASE(KwNull)
        HAMMER_CASE(KwImport)
        HAMMER_CASE(KwExport)
        HAMMER_CASE(KwPackage)

        HAMMER_CASE(KwYield)
        HAMMER_CASE(KwAsync)
        HAMMER_CASE(KwAwait)
        HAMMER_CASE(KwThrow)
        HAMMER_CASE(KwTry)
        HAMMER_CASE(KwCatch)
        HAMMER_CASE(KwScope)

        HAMMER_CASE(LParen)
        HAMMER_CASE(RParen)
        HAMMER_CASE(LBracket)
        HAMMER_CASE(RBracket)
        HAMMER_CASE(LBrace)
        HAMMER_CASE(RBrace)

        HAMMER_CASE(Dot)
        HAMMER_CASE(Comma)
        HAMMER_CASE(Colon)
        HAMMER_CASE(Semicolon)
        HAMMER_CASE(Question)
        HAMMER_CASE(Plus)
        HAMMER_CASE(Minus)
        HAMMER_CASE(Star)
        HAMMER_CASE(Starstar)
        HAMMER_CASE(Slash)
        HAMMER_CASE(Percent)
        HAMMER_CASE(PlusPlus)
        HAMMER_CASE(MinusMinus)
        HAMMER_CASE(BNot)
        HAMMER_CASE(BOr)
        HAMMER_CASE(BXor)
        HAMMER_CASE(BAnd)
        HAMMER_CASE(LNot)
        HAMMER_CASE(LOr)
        HAMMER_CASE(LAnd)
        HAMMER_CASE(Eq)
        HAMMER_CASE(EqEq)
        HAMMER_CASE(NEq)
        HAMMER_CASE(Less)
        HAMMER_CASE(Greater)
        HAMMER_CASE(LessEq)
        HAMMER_CASE(GreaterEq)

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
        HAMMER_CASE_Q(KwTrue, "true")
        HAMMER_CASE_Q(KwFalse, "false")
        HAMMER_CASE_Q(KwNull, "null")
        HAMMER_CASE_Q(KwImport, "import")
        HAMMER_CASE_Q(KwExport, "export")
        HAMMER_CASE_Q(KwPackage, "package")

        HAMMER_CASE_Q(KwYield, "yield")
        HAMMER_CASE_Q(KwAsync, "async")
        HAMMER_CASE_Q(KwAwait, "await")
        HAMMER_CASE_Q(KwThrow, "throw")
        HAMMER_CASE_Q(KwTry, "try")
        HAMMER_CASE_Q(KwCatch, "catch")
        HAMMER_CASE_Q(KwScope, "scope")

        HAMMER_CASE_Q(LParen, "(")
        HAMMER_CASE_Q(RParen, ")")
        HAMMER_CASE_Q(LBracket, "[")
        HAMMER_CASE_Q(RBracket, "]")
        HAMMER_CASE_Q(LBrace, "{")
        HAMMER_CASE_Q(RBrace, "}")

        HAMMER_CASE_Q(Dot, ".")
        HAMMER_CASE_Q(Comma, ",")
        HAMMER_CASE_Q(Colon, ":")
        HAMMER_CASE_Q(Semicolon, ";")
        HAMMER_CASE_Q(Question, "?")
        HAMMER_CASE_Q(Plus, "+")
        HAMMER_CASE_Q(Minus, "-")
        HAMMER_CASE_Q(Star, "*")
        HAMMER_CASE_Q(Starstar, "**")
        HAMMER_CASE_Q(Slash, "/")
        HAMMER_CASE_Q(Percent, "%")
        HAMMER_CASE_Q(PlusPlus, "++")
        HAMMER_CASE_Q(MinusMinus, "--")
        HAMMER_CASE_Q(BNot, "~")
        HAMMER_CASE_Q(BOr, "|")
        HAMMER_CASE_Q(BXor, "^")
        HAMMER_CASE_Q(BAnd, "&")
        HAMMER_CASE_Q(LNot, "!")
        HAMMER_CASE_Q(LOr, "||")
        HAMMER_CASE_Q(LAnd, "&&")
        HAMMER_CASE_Q(Eq, "=")
        HAMMER_CASE_Q(EqEq, "==")
        HAMMER_CASE_Q(NEq, "!=")
        HAMMER_CASE_Q(Less, "<")
        HAMMER_CASE_Q(Greater, ">")
        HAMMER_CASE_Q(LessEq, "<=")
        HAMMER_CASE_Q(GreaterEq, ">=")

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

double Token::float_value() const {
    HAMMER_ASSERT(std::holds_alternative<double>(value_),
        "Token does not contain a float value.");
    return std::get<double>(value_);
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
