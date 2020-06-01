#include "tiro/ast/token.hpp"

#include "tiro/core/defs.hpp"

namespace tiro {

std::string_view to_token_name(TokenType tok) {
    switch (tok) {
#define TIRO_CASE(x)     \
    case (TokenType::x): \
        return #x;

        TIRO_CASE(InvalidToken)
        TIRO_CASE(Eof)
        TIRO_CASE(Comment)

        TIRO_CASE(Identifier)
        TIRO_CASE(SymbolLiteral)
        TIRO_CASE(StringContent)
        TIRO_CASE(FloatLiteral)
        TIRO_CASE(IntegerLiteral)
        TIRO_CASE(NumericMember)

        TIRO_CASE(KwFunc)
        TIRO_CASE(KwVar)
        TIRO_CASE(KwConst)
        TIRO_CASE(KwIs)
        TIRO_CASE(KwAs)
        TIRO_CASE(KwIn)
        TIRO_CASE(KwIf)
        TIRO_CASE(KwElse)
        TIRO_CASE(KwWhile)
        TIRO_CASE(KwFor)
        TIRO_CASE(KwContinue)
        TIRO_CASE(KwBreak)
        TIRO_CASE(KwReturn)
        TIRO_CASE(KwSwitch)
        TIRO_CASE(KwClass)
        TIRO_CASE(KwStruct)
        TIRO_CASE(KwProtocol)
        TIRO_CASE(KwAssert)
        TIRO_CASE(KwTrue)
        TIRO_CASE(KwFalse)
        TIRO_CASE(KwNull)
        TIRO_CASE(KwImport)
        TIRO_CASE(KwExport)
        TIRO_CASE(KwPackage)
        TIRO_CASE(KwMap)
        TIRO_CASE(KwSet)

        TIRO_CASE(KwYield)
        TIRO_CASE(KwAsync)
        TIRO_CASE(KwAwait)
        TIRO_CASE(KwThrow)
        TIRO_CASE(KwTry)
        TIRO_CASE(KwCatch)
        TIRO_CASE(KwScope)

        TIRO_CASE(LeftParen)
        TIRO_CASE(RightParen)
        TIRO_CASE(LeftBracket)
        TIRO_CASE(RightBracket)
        TIRO_CASE(LeftBrace)
        TIRO_CASE(RightBrace)

        TIRO_CASE(Dot)
        TIRO_CASE(Comma)
        TIRO_CASE(Colon)
        TIRO_CASE(Semicolon)
        TIRO_CASE(Question)
        TIRO_CASE(Plus)
        TIRO_CASE(Minus)
        TIRO_CASE(Star)
        TIRO_CASE(StarStar)
        TIRO_CASE(Slash)
        TIRO_CASE(Percent)
        TIRO_CASE(PlusEquals)
        TIRO_CASE(MinusEquals)
        TIRO_CASE(StarEquals)
        TIRO_CASE(StarStarEquals)
        TIRO_CASE(SlashEquals)
        TIRO_CASE(PercentEquals)
        TIRO_CASE(PlusPlus)
        TIRO_CASE(MinusMinus)
        TIRO_CASE(BitwiseNot)
        TIRO_CASE(BitwiseOr)
        TIRO_CASE(BitwiseXor)
        TIRO_CASE(BitwiseAnd)
        TIRO_CASE(LeftShift)
        TIRO_CASE(RightShift)
        TIRO_CASE(LogicalNot)
        TIRO_CASE(LogicalOr)
        TIRO_CASE(LogicalAnd)
        TIRO_CASE(Equals)
        TIRO_CASE(EqualsEquals)
        TIRO_CASE(NotEquals)
        TIRO_CASE(Less)
        TIRO_CASE(Greater)
        TIRO_CASE(LessEquals)
        TIRO_CASE(GreaterEquals)

        TIRO_CASE(Dollar)
        TIRO_CASE(DollarLeftBrace)
        TIRO_CASE(SingleQuote)
        TIRO_CASE(DoubleQuote)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid token type");
}

std::string_view to_description(TokenType tok) {
    switch (tok) {
#define TIRO_CASE(x, s) \
    case TokenType::x:  \
        return s;

#define TIRO_CASE_Q(x, s) TIRO_CASE(x, "'" s "'")

        TIRO_CASE(InvalidToken, "<invalid_token>")
        TIRO_CASE(Eof, "<end of file>")
        TIRO_CASE(Comment, "<comment>")

        TIRO_CASE(Identifier, "<identifier>")
        TIRO_CASE(SymbolLiteral, "<symbol>")
        TIRO_CASE(StringContent, "<string>")
        TIRO_CASE(FloatLiteral, "<float>")
        TIRO_CASE(IntegerLiteral, "<integer>")
        TIRO_CASE(NumericMember, "<numeric member>")

        TIRO_CASE_Q(KwFunc, "func")
        TIRO_CASE_Q(KwVar, "var")
        TIRO_CASE_Q(KwConst, "const")
        TIRO_CASE_Q(KwIs, "is")
        TIRO_CASE_Q(KwAs, "as")
        TIRO_CASE_Q(KwIn, "in")
        TIRO_CASE_Q(KwIf, "if")
        TIRO_CASE_Q(KwElse, "else")
        TIRO_CASE_Q(KwWhile, "while")
        TIRO_CASE_Q(KwFor, "for")
        TIRO_CASE_Q(KwContinue, "continue")
        TIRO_CASE_Q(KwBreak, "break")
        TIRO_CASE_Q(KwReturn, "return")
        TIRO_CASE_Q(KwSwitch, "switch")
        TIRO_CASE_Q(KwClass, "class")
        TIRO_CASE_Q(KwStruct, "struct")
        TIRO_CASE_Q(KwProtocol, "protocol")
        TIRO_CASE_Q(KwAssert, "assert")
        TIRO_CASE_Q(KwTrue, "true")
        TIRO_CASE_Q(KwFalse, "false")
        TIRO_CASE_Q(KwNull, "null")
        TIRO_CASE_Q(KwImport, "import")
        TIRO_CASE_Q(KwExport, "export")
        TIRO_CASE_Q(KwPackage, "package")
        TIRO_CASE_Q(KwMap, "Map")
        TIRO_CASE_Q(KwSet, "Set")

        TIRO_CASE_Q(KwYield, "yield")
        TIRO_CASE_Q(KwAsync, "async")
        TIRO_CASE_Q(KwAwait, "await")
        TIRO_CASE_Q(KwThrow, "throw")
        TIRO_CASE_Q(KwTry, "try")
        TIRO_CASE_Q(KwCatch, "catch")
        TIRO_CASE_Q(KwScope, "scope")

        TIRO_CASE_Q(LeftParen, "(")
        TIRO_CASE_Q(RightParen, ")")
        TIRO_CASE_Q(LeftBracket, "[")
        TIRO_CASE_Q(RightBracket, "]")
        TIRO_CASE_Q(LeftBrace, "{")
        TIRO_CASE_Q(RightBrace, "}")

        TIRO_CASE_Q(Dot, ".")
        TIRO_CASE_Q(Comma, ",")
        TIRO_CASE_Q(Colon, ":")
        TIRO_CASE_Q(Semicolon, ";")
        TIRO_CASE_Q(Question, "?")
        TIRO_CASE_Q(Plus, "+")
        TIRO_CASE_Q(Minus, "-")
        TIRO_CASE_Q(Star, "*")
        TIRO_CASE_Q(StarStar, "**")
        TIRO_CASE_Q(Slash, "/")
        TIRO_CASE_Q(Percent, "%")
        TIRO_CASE_Q(PlusEquals, "+=")
        TIRO_CASE_Q(MinusEquals, "-=")
        TIRO_CASE_Q(StarEquals, "*=")
        TIRO_CASE_Q(StarStarEquals, "**=")
        TIRO_CASE_Q(SlashEquals, "/=")
        TIRO_CASE_Q(PercentEquals, "%=")
        TIRO_CASE_Q(PlusPlus, "++")
        TIRO_CASE_Q(MinusMinus, "--")
        TIRO_CASE_Q(BitwiseNot, "~")
        TIRO_CASE_Q(BitwiseOr, "|")
        TIRO_CASE_Q(BitwiseXor, "^")
        TIRO_CASE_Q(BitwiseAnd, "&")
        TIRO_CASE_Q(LeftShift, "<<")
        TIRO_CASE_Q(RightShift, ">>")
        TIRO_CASE_Q(LogicalNot, "!")
        TIRO_CASE_Q(LogicalOr, "||")
        TIRO_CASE_Q(LogicalAnd, "&&")
        TIRO_CASE_Q(Equals, "=")
        TIRO_CASE_Q(EqualsEquals, "==")
        TIRO_CASE_Q(NotEquals, "!=")
        TIRO_CASE_Q(Less, "<")
        TIRO_CASE_Q(Greater, ">")
        TIRO_CASE_Q(LessEquals, "<=")
        TIRO_CASE_Q(GreaterEquals, ">=")

        TIRO_CASE_Q(Dollar, "$")
        TIRO_CASE_Q(DollarLeftBrace, "${")
        TIRO_CASE_Q(SingleQuote, "'")
        TIRO_CASE_Q(DoubleQuote, "\"")

#undef TIRO_CASE
#undef TIRO_CASE_Q
    }

    TIRO_UNREACHABLE("Invalid token type");
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ast import TokenData
    implement(TokenData.tag, TokenData)
]]] */
std::string_view to_string(TokenDataType type) {
    switch (type) {
    case TokenDataType::None:
        return "None";
    case TokenDataType::Integer:
        return "Integer";
    case TokenDataType::Float:
        return "Float";
    case TokenDataType::String:
        return "String";
    }
    TIRO_UNREACHABLE("Invalid TokenDataType.");
}

TokenData TokenData::make_none() {
    return {None{}};
}

TokenData TokenData::make_integer(const Integer& integer) {
    return integer;
}

TokenData TokenData::make_float(const Float& f) {
    return f;
}

TokenData TokenData::make_string(const String& string) {
    return string;
}

TokenData::TokenData(None none)
    : type_(TokenDataType::None)
    , none_(std::move(none)) {}

TokenData::TokenData(Integer integer)
    : type_(TokenDataType::Integer)
    , integer_(std::move(integer)) {}

TokenData::TokenData(Float f)
    : type_(TokenDataType::Float)
    , float_(std::move(f)) {}

TokenData::TokenData(String string)
    : type_(TokenDataType::String)
    , string_(std::move(string)) {}

const TokenData::None& TokenData::as_none() const {
    TIRO_DEBUG_ASSERT(type_ == TokenDataType::None, "Bad member access on TokenData: not a None.");
    return none_;
}

const TokenData::Integer& TokenData::as_integer() const {
    TIRO_DEBUG_ASSERT(
        type_ == TokenDataType::Integer, "Bad member access on TokenData: not a Integer.");
    return integer_;
}

const TokenData::Float& TokenData::as_float() const {
    TIRO_DEBUG_ASSERT(
        type_ == TokenDataType::Float, "Bad member access on TokenData: not a Float.");
    return float_;
}

const TokenData::String& TokenData::as_string() const {
    TIRO_DEBUG_ASSERT(
        type_ == TokenDataType::String, "Bad member access on TokenData: not a String.");
    return string_;
}

// [[[end]]]

} // namespace tiro
