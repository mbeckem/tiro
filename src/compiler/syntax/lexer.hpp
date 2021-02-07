#ifndef TIRO_COMPILER_SYNTAX_LEXER_HPP
#define TIRO_COMPILER_SYNTAX_LEXER_HPP

#include "common/text/code_point_range.hpp"
#include "compiler/syntax/token.hpp"

#include <string_view>
#include <variant>
#include <vector>

namespace tiro::next {

/// The lexer splits the source code into tokens.
/// TODO: Introduce whitespace tokens to preserve all input.
class Lexer final {
public:
    Lexer(std::string_view file_content);

    /// If true, comments will not be returned as tokens (they are skipped, unless
    /// they contain an error).
    /// Defaults to true.
    void ignore_comments(bool ignore) { ignore_comments_ = ignore; }
    bool ignore_comments() const { return ignore_comments_; }

    /// Index of the current character.
    size_t pos() const;

    /// Seek to the given position.
    void pos(size_t pos);

    /// Returns the next token from the current position within the source text.
    Token next();

private:
    enum StateType { Normal, String };

    struct State {
        StateType type = Normal;

        // State == Normal
        u32 open_braces = 0;

        // State == String
        CodePoint string_delim = 0;
        bool string_needs_identifier = false;

        static State normal() {
            State st;
            st.type = Normal;
            return st;
        }

        static State string(CodePoint delim) {
            State st;
            st.type = String;
            st.string_delim = delim;
            return st;
        }
    };

    TokenType lex_normal();
    TokenType lex_identifier();
    TokenType lex_symbol();
    TokenType lex_number();
    TokenType lex_tuple_field();
    std::optional<TokenType> lex_operator();

    void lex_line_comment();
    void lex_block_comment();

    TokenType lex_string();
    void lex_string_content();

    // Returns the source text that was accepted as part of this token so far.
    std::string_view value() const;

    // Byte offset of the next character.
    size_t next_pos() const;

    // Advance unconditionally.
    void advance();
    void advance(size_t n);

    // True if at the end of file.
    bool eof() const;

    // Returns the current code point. Must not be at eof.
    CodePoint current() const;

    // Returns the nth code point from the current one. `ahead(0)` is the same as `current()`.
    std::optional<CodePoint> ahead(size_t n);

    // Advances the range if the current code point is equal to `c` and returns true in that case.
    // Does nothing and returns false otherwise.
    bool accept(CodePoint c);

    // Accepts one of the candidate code points and returns it.
    // Returns an empty optional if the current code point does not match any of the candidates.
    std::optional<CodePoint> accept_any(std::initializer_list<CodePoint> candidates);

    /// Accepts characters while not at eof and while the predicate returns true.
    template<typename Func>
    void accept_while(Func&& predicate);

    // Skips all code points until the current one is not equal to `c`
    void skip_until(CodePoint c);

    void push_state();
    bool pop_state();

private:
    std::string_view file_content_;
    bool ignore_comments_ = true;
    CodePointRange input_;

    // Start offset of the current token.
    size_t start_ = 0;

    // Last emitted token (non whitespace / comment), used to disambiguate floats from tuple field accesses.
    TokenType last_non_ws_ = TokenType::Eof;

    // Stack of saved states.
    std::vector<State> states_;

    // The current lexer state.
    State state_ = State::normal();
};

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_LEXER_HPP
