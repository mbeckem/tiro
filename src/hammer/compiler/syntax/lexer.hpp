#ifndef HAMMER_COMPILER_SYNTAX_LEXER_HPP
#define HAMMER_COMPILER_SYNTAX_LEXER_HPP

#include "hammer/compiler/syntax/token.hpp"
#include "hammer/core/code_point_range.hpp"

#include <string_view>
#include <unordered_map>

namespace hammer::compiler {

class Diagnostics;
class StringTable;

enum class LexerMode {
    /// Default mode
    Normal,

    /// Active when the parser attempts to parse a member expr, i.e. EXPR "." MEMBER.
    /// In this mode, number parsing is handled differently to make expressions like FOO.0.1.2 possible.
    Member
};

// TODO: Lexer modes for normal/string parsing to support format strings (e.g. `${hello}`).
class Lexer final {
public:
    Lexer(InternedString file_name, std::string_view file_content,
        StringTable& strings, Diagnostics& diag);

    InternedString file_name() const { return file_name_; }
    std::string_view file_content() const { return file_content_; }
    StringTable& strings() const { return strings_; }
    Diagnostics& diag() const { return diag_; }

    // If true, comments will not be returned as tokens (they are skipped, unless
    // they contain an error).
    // Defaults to true.
    void ignore_comments(bool ignore) { ignore_comments_ = ignore; }
    bool ignore_comments() const { return ignore_comments_; }

    LexerMode mode() const { return mode_; }
    void mode(LexerMode mode) { mode_ = mode; }

    // Returns the next token from the current position within the source text.
    Token next();

private:
    Token lex_name();
    Token lex_symbol();
    Token lex_string();
    Token lex_number();
    Token lex_numeric_member();

    std::optional<Token> lex_operator();

    Token lex_line_comment();
    Token lex_block_comment();

private:
    // Index of the current character.
    size_t pos() const;

    // Index of the next character.
    size_t next_pos() const;

    // Returns a source reference from the given index (inclusive) to the current character (exclusive).
    SourceReference ref(size_t begin) const;

    // Returns a source reference to [begin, end) of the input.
    SourceReference ref(size_t begin, size_t end) const;

    // Literal source code [begin, end).
    std::string_view substr(size_t begin, size_t end) const;

    // Skips all code points until the current one is not equal to `c`
    void skip(CodePoint c);

private:
    StringTable& strings_;
    InternedString file_name_;
    std::string_view file_content_;
    Diagnostics& diag_;
    LexerMode mode_ = LexerMode::Normal;

    bool ignore_comments_ = true;

    // Iterates over the file content.
    CodePointRange input_;

    // Maps interned string values (names/identiifers) to keywords.
    std::unordered_map<InternedString, TokenType, UseHasher> keywords_;

    // For parsing string data.
    std::string buffer_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_SYNTAX_LEXER_HPP
