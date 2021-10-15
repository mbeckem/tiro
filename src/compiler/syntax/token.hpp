#ifndef TIRO_COMPILER_SYNTAX_TOKEN_HPP
#define TIRO_COMPILER_SYNTAX_TOKEN_HPP

#include "common/format.hpp"
#include "compiler/source_range.hpp"

#include <string_view>

namespace tiro {

/// List of all known tokens.
///
/// Note: if you add a new keyword, you will likely want to
/// add the string --> token_type mapping in lexer.cpp (keywords_table) as well.
enum class TokenType : u8 {
    Unexpected = 0, // Unexpected character
    Eof,
    Comment,

    // Primitives
    Identifier, // ordinary variable names
    Symbol,     // #name
    Integer,    // 0 1 0x123 0b0100 0o456
    Float,      // 123.456
    TupleField, // tuple index after dot, e.g. the "1" in "a.1"

    // Strings
    StringStart,      // ' or "
    StringContent,    // raw string literal content
    StringVar,        // $       single identifier follows
    StringBlockStart, // ${      terminated by StringBlockEnd
    StringBlockEnd,   // }
    StringEnd,        // matching ' or "

    // Keywords
    KwAssert,
    KwBreak,
    KwConst,
    KwContinue,
    KwDefer,
    KwElse,
    KwExport,
    KwFalse,
    KwFor,
    KwFunc,
    KwIf,
    KwImport,
    KwIn,
    KwNull,
    KwReturn,
    KwTrue,
    KwVar,
    KwWhile,

    // Contextual keywords
    KwMap,
    KwSet,

    // Reserved
    KwAs,
    KwCatch,
    KwClass,
    KwInterface,
    KwIs,
    KwPackage,
    KwProtocol,
    KwScope,
    KwStruct,
    KwSwitch,
    KwThrow,
    KwTry,
    KwYield,

    // Braces
    LeftParen,    // (
    RightParen,   // )
    LeftBracket,  // [
    RightBracket, // ]
    LeftBrace,    // {
    RightBrace,   // }

    // Operators
    Dot,                 // .
    Comma,               // ,
    Colon,               // :
    Semicolon,           // ;
    Question,            // ?
    QuestionDot,         // ?.
    QuestionLeftParen,   // ?(
    QuestionLeftBracket, // ?[
    QuestionQuestion,    // ??
    Plus,                // +
    Minus,               // -
    Star,                // *
    StarStar,            // **
    Slash,               // /
    Percent,             // %
    PlusEquals,          // +=
    MinusEquals,         // -=
    StarEquals,          // *=
    StarStarEquals,      // **=
    SlashEquals,         // /=
    PercentEquals,       // %=
    PlusPlus,            // ++
    MinusMinus,          // --
    BitwiseNot,          // ~
    BitwiseOr,           // |
    BitwiseXor,          // ^
    BitwiseAnd,          // &
    LeftShift,           // <<
    RightShift,          // >>
    LogicalNot,          // !
    LogicalOr,           // ||
    LogicalAnd,          // &&
    Equals,              // =
    EqualsEquals,        // ==
    NotEquals,           // !=
    Less,                // <
    Greater,             // >
    LessEquals,          // <=
    GreaterEquals,       // >=

    // Must keep in sync with largest value!
    MAX_VALUE = GreaterEquals
};

/// Returns the name of the enum identifier.
std::string_view to_string(TokenType tok);

/// Returns a human readable string for the given token.
std::string_view to_description(TokenType tok);

/// Returns the raw numeric value of the given token type.
constexpr auto to_underlying(TokenType type) {
    return static_cast<std::underlying_type_t<TokenType>>(type);
}

/// A token represents a section of source code text together with its lexical type.
/// Tokens are returned by the lexer.
class Token final {
public:
    Token() = default;

    Token(TokenType type, const SourceRange& range)
        : type_(type)
        , range_(range) {}

    /// Type of the token.
    TokenType type() const { return type_; }
    void type(TokenType t) { type_ = t; }

    /// Source code part that contains the token.
    const SourceRange& range() const { return range_; }
    void range(const SourceRange& range) { range_ = range; }

    void format(FormatStream& stream) const;

private:
    TokenType type_ = TokenType::Unexpected;
    SourceRange range_;
};

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::TokenType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::Token)

#endif // TIRO_COMPILER_SYNTAX_TOKEN_HPP
