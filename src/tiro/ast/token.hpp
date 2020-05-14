#ifndef TIRO_AST_TOKEN_HPP
#define TIRO_AST_TOKEN_HPP

#include "tiro/compiler/source_reference.hpp"
#include "tiro/core/string_table.hpp"

#include <bitset>
#include <string_view>
#include <variant>

namespace tiro {

/// List of all known tokens.
///
/// Note: if you add a new keyword, you will likely want to
/// add the string --> token_type mapping in lexer.cpp (keywords_table) as well.
enum class TokenType : u8 {
    InvalidToken = 0,
    Eof,
    Comment,

    // Primitives
    Identifier,     // ordinary variable names
    SymbolLiteral,  // #name
    StringContent,  // literal content
    FloatLiteral,   // 123.456
    IntegerLiteral, // 0 1 0x123 0b0100 0o456
    NumericMember,  // requires lexer mode, for tuple members

    // Keywords
    KwFunc,
    KwVar,
    KwConst,
    KwIs,
    KwAs,
    KwIn,
    KwIf,
    KwElse,
    KwWhile,
    KwFor,
    KwContinue,
    KwBreak,
    KwReturn,
    KwSwitch,
    KwClass,
    KwStruct,
    KwProtocol,
    KwAssert,
    KwTrue,
    KwFalse,
    KwNull,
    KwImport,
    KwExport,
    KwPackage,

    // TODO Move this into the type system instead?
    KwMap, // Map (uppercase)
    KwSet, // Set (uppercase)

    // Reserved
    KwYield,
    KwAsync,
    KwAwait,
    KwThrow,
    KwTry,
    KwCatch,
    KwScope,

    // Braces
    LeftParen,    // (
    RightParen,   // )
    LeftBracket,  // [
    RightBracket, // ]
    LeftBrace,    // {
    RightBrace,   // }

    // Operators
    Dot,             // .
    Comma,           // ,
    Colon,           // :
    Semicolon,       // ;
    Question,        // ?
    Plus,            // +
    Minus,           // -
    Star,            // *
    StarStar,        // **
    Slash,           // /
    Percent,         // %
    PlusEquals,      // +=
    MinusEquals,     // -=
    StarEquals,      // *=
    StarStarEquals,  // **=
    SlashEquals,     // /=
    PercentEquals,   // %=
    PlusPlus,        // ++
    MinusMinus,      // --
    BitwiseNot,      // ~
    BitwiseOr,       // |
    BitwiseXor,      // ^
    BitwiseAnd,      // &
    LeftShift,       // <<
    RightShift,      // >>
    LogicalNot,      // !
    LogicalOr,       // ||
    LogicalAnd,      // &&
    Equals,          // =
    EqualsEquals,    // ==
    NotEquals,       // !=
    Less,            // <
    Greater,         // >
    LessEquals,      // <=
    GreaterEquals,   // >=
    Dollar,          // $
    DollarLeftBrace, // ${
    DoubleQuote,     // "
    SingleQuote,     // '

    // Must keep in sync with largest value!
    MaxEnumValue = SingleQuote
};

// Returns the name of the enum identifier.
std::string_view to_token_name(TokenType tok);

// Returns a human readable string for the given token.
std::string_view to_description(TokenType tok);

// Returns the raw numeric value of the given token type.
constexpr auto to_underlying(TokenType type) {
    return static_cast<std::underlying_type_t<TokenType>>(type);
}

/* [[[cog
    from codegen.unions import define
    from codegen.ast import TokenData
    define(TokenData.tag)
]]] */
enum class TokenDataType : u8 {
    None,
    Integer,
    Float,
    String,
};

std::string_view to_string(TokenDataType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import TokenData
    define(TokenData)
]]] */
/// Represents data associated with a token.
class TokenData final {
public:
    /// No additional value at all (the most common case).
    struct None final {};

    using Integer = i64;

    using Float = f64;

    using String = InternedString;

    static TokenData make_none();
    static TokenData make_integer(integer);
    static TokenData make_float(f);
    static TokenData make_string(string);

    TokenData(None none);
    TokenData(Integer integer);
    TokenData(Float f);
    TokenData(String string);

    TokenDataType type() const noexcept { return type_; }

    const None& as_none() const;
    const Integer& as_integer() const;
    const Float& as_float() const;
    const String& as_string() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto)
    visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    TokenDataType type_;
    union {
        None none_;
        Integer integer_;
        Float float_;
        String string_;
    };
};
// [[[end]]]

class Token final {
public:
    Token() = default;

    Token(TokenType type, const SourceReference& source)
        : type_(type)
        , source_(source) {}

    // Type of the token.
    TokenType type() const { return type_; }
    void type(TokenType t) { type_ = t; }

    // Source code part that contains the token.
    const SourceReference& source() const { return source_; }
    void source(const SourceReference& source) { source_ = source; }

    // True if the Token contains an error (e.g. invalid characters within a
    // number or an identifier).
    bool has_error() const { return has_error_; }
    void has_error(bool has_error) { has_error_ = has_error; }

    const TokenData& data() const { return data_; }
    void data(const TokenData& data) { data_ = data; }

private:
    TokenType type_ = TokenType::InvalidToken;
    bool has_error_ = false;
    SourceReference source_;
    TokenData data_ = TokenData::make_none();
};

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.ast import TokenData
    implement_inlines(TokenData)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto)
TokenData::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case TokenDataType::None:
        return vis.visit_none(self.none_, std::forward<Args>(args)...);
    case TokenDataType::Integer:
        return vis.visit_integer(self.integer_, std::forward<Args>(args)...);
    case TokenDataType::Float:
        return vis.visit_float(self.float_, std::forward<Args>(args)...);
    case TokenDataType::String:
        return vis.visit_string(self.string_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid TokenData type.");
}
/// [[[end]]]

} // namespace tiro

#endif // TIRO_AST_TOKEN_HPP
