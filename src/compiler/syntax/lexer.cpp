#include "compiler/syntax/lexer.hpp"

#include "common/iter_tools.hpp"
#include "common/math.hpp"
#include "common/safe_int.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/syntax/literals.hpp"

#include "absl/container/flat_hash_map.h"

namespace tiro::next {

// TODO: Init concerns, maybe use a constexpr hash table here?
static const absl::flat_hash_map<std::string_view, TokenType> keywords = {
    {"func", TokenType::KwFunc},
    {"var", TokenType::KwVar},
    {"const", TokenType::KwConst},
    {"is", TokenType::KwIs},
    {"as", TokenType::KwAs},
    {"in", TokenType::KwIn},
    {"if", TokenType::KwIf},
    {"else", TokenType::KwElse},
    {"while", TokenType::KwWhile},
    {"for", TokenType::KwFor},
    {"continue", TokenType::KwContinue},
    {"break", TokenType::KwBreak},
    {"return", TokenType::KwReturn},
    {"switch", TokenType::KwSwitch},
    {"class", TokenType::KwClass},
    {"struct", TokenType::KwStruct},
    {"protocol", TokenType::KwProtocol},
    {"assert", TokenType::KwAssert},
    {"true", TokenType::KwTrue},
    {"false", TokenType::KwFalse},
    {"null", TokenType::KwNull},
    {"import", TokenType::KwImport},
    {"export", TokenType::KwExport},
    {"package", TokenType::KwPackage},
    {"yield", TokenType::KwYield},
    {"async", TokenType::KwAsync},
    {"await", TokenType::KwAwait},
    {"throw", TokenType::KwThrow},
    {"try", TokenType::KwTry},
    {"catch", TokenType::KwCatch},
    {"scope", TokenType::KwScope},
    {"defer", TokenType::KwDefer},
};

static bool is_decimal_digit(CodePoint c) {
    return c >= '0' && c <= '9';
}

static bool is_identifier_begin(CodePoint c) {
    return is_letter(c) || c == '_';
}

static bool is_identifier_part(CodePoint c) {
    return is_identifier_begin(c) || is_number(c);
}

Lexer::Lexer(std::string_view file_content)
    : file_content_(file_content)
    , input_(file_content) {}

Token Lexer::next() {
    start_ = pos();

    const auto next_token_type = [&] {
        switch (state_.type) {
        case Normal:
            return lex_normal();
        case String:
            return lex_string();
        }
        TIRO_UNREACHABLE("Invalid state type.");
    }();

    if (next_token_type != TokenType::Comment) {
        last_non_ws_ = next_token_type;
    }
    return Token(next_token_type, SourceRange(start_, pos()));
}

TokenType Lexer::lex_normal() {
    TIRO_DEBUG_ASSERT(state_.type == Normal, "Invalid state.");

again:
    // Skip whitespace
    accept_while(is_whitespace);
    start_ = pos();

    if (eof())
        return TokenType::Eof;

    CodePoint c = current();

    if (c == '/' && ahead(1) == '/') {
        lex_line_comment();
        if (ignore_comments_)
            goto again;
        return TokenType::Comment;
    }

    if (c == '/' && ahead(1) == '*') {
        lex_block_comment();
        if (ignore_comments_)
            goto again;
        return TokenType::Comment;
    }

    if (c == '\'' || c == '"') {
        advance();
        push_state();
        state_ = State::string(c);
        return TokenType::StringStart;
    }

    if (is_decimal_digit(c))
        return last_non_ws_ == TokenType::Dot ? lex_tuple_field() : lex_number();

    if (c == '#')
        return lex_symbol();

    if (is_identifier_begin(c))
        return lex_identifier();

    if (auto op = lex_operator()) {
        switch (*op) {
        case TokenType::LeftBrace:
            ++state_.open_braces;
            break;
        case TokenType::RightBrace: {
            // Transition back to string state
            if (state_.open_braces == 0 && pop_state())
                return TokenType::StringBlockEnd;

            // Topmost state can have additional closing braces
            if (state_.open_braces > 0)
                --state_.open_braces;
            break;
        }
        default:
            break;
        }
        return *op;
    }

    advance();
    return TokenType::Unexpected;
}

TokenType Lexer::lex_identifier() {
    TIRO_DEBUG_ASSERT(is_identifier_begin(current()), "Not at the start of an identifier.");

    advance();
    accept_while(is_identifier_part);

    auto kw_it = keywords.find(value());
    if (kw_it != keywords.end())
        return kw_it->second;

    return TokenType::Identifier;
}

TokenType Lexer::lex_number() {
    TIRO_DEBUG_ASSERT(is_decimal_digit(current()), "Not at the start of a number.");

    // More relaxed base for parsing (-> better error messages for digits)
    int parse_base = 10;
    auto is_digit_char = [&](CodePoint c) {
        return c == '_' || to_digit(c, parse_base).has_value();
    };

    // Determine the base of the number literal.
    if (accept('0')) {
        if (auto c = accept_any({'b', 'o', 'x'}); c == 'x') {
            parse_base = 16;
        }
    }

    // Parse the integer part of the number literal
    accept_while(is_digit_char);
    if (accept('.')) {
        accept_while(is_digit_char);
        // TODO: Exponents
        return TokenType::Float;
    }
    return TokenType::Integer;
}

TokenType Lexer::lex_tuple_field() {
    TIRO_DEBUG_ASSERT(is_decimal_digit(current()), "Not at the start of a tuple field.");

    accept_while(is_decimal_digit);
    return TokenType::TupleField;
}

TokenType Lexer::lex_symbol() {
    TIRO_DEBUG_ASSERT(current() == '#', "Not at the start of a symbol.");

    advance();
    accept_while(is_identifier_part);
    return TokenType::Symbol;
}

std::optional<TokenType> Lexer::lex_operator() {
    TIRO_DEBUG_ASSERT(!eof(), "Already at the end of file.");

    switch (current()) {

#define TIRO_OP(c, ...) \
    case c: {           \
        advance();      \
        __VA_ARGS__     \
    }

        // Braces
        TIRO_OP('(', return TokenType::LeftParen;)
        TIRO_OP(')', return TokenType::RightParen;)
        TIRO_OP('[', return TokenType::LeftBracket;)
        TIRO_OP(']', return TokenType::RightBracket;)
        TIRO_OP('{', return TokenType::LeftBrace;)
        TIRO_OP('}', return TokenType::RightBrace;)

        // Operators
        TIRO_OP('.', return TokenType::Dot;)
        TIRO_OP(',', return TokenType::Comma;)
        TIRO_OP(':', return TokenType::Colon;)
        TIRO_OP(';', return TokenType::Semicolon;)
        TIRO_OP('?', {
            switch (current()) {
                TIRO_OP('.', return TokenType::QuestionDot;)
                TIRO_OP('(', return TokenType::QuestionLeftParen;)
                TIRO_OP('[', return TokenType::QuestionLeftBracket;)
                TIRO_OP('?', return TokenType::QuestionQuestion;)
            default:
                return TokenType::Question;
            }
        })
        TIRO_OP('+', {
            switch (current()) {
                TIRO_OP('+', return TokenType::PlusPlus;)
                TIRO_OP('=', return TokenType::PlusEquals;)
            default:
                return TokenType::Plus;
            }
        })
        TIRO_OP('-', {
            switch (current()) {
                TIRO_OP('-', return TokenType::MinusMinus;)
                TIRO_OP('=', return TokenType::MinusEquals;)
            default:
                return TokenType::Minus;
            }
        })
        TIRO_OP('*', {
            switch (current()) {
                TIRO_OP('*', {
                    if (accept('='))
                        return TokenType::StarStarEquals;
                    return TokenType::StarStar;
                })
                TIRO_OP('=', return TokenType::StarEquals;)
            default:
                return TokenType::Star;
            }
        })
        TIRO_OP('/', {
            if (accept('='))
                return TokenType::SlashEquals;
            return TokenType::Slash;
        })
        TIRO_OP('%', {
            if (accept('='))
                return TokenType::PercentEquals;
            return TokenType::Percent;
        })
        TIRO_OP('~', return TokenType::BitwiseNot;)
        TIRO_OP('^', return TokenType::BitwiseXor;)
        TIRO_OP('!', {
            if (accept('='))
                return TokenType::NotEquals;
            return TokenType::LogicalNot;
        })
        TIRO_OP('|', {
            if (accept('|'))
                return TokenType::LogicalOr;
            return TokenType::BitwiseOr;
        })
        TIRO_OP('&', {
            if (accept('&'))
                return TokenType::LogicalAnd;
            return TokenType::BitwiseAnd;
        })
        TIRO_OP('=', {
            if (accept('='))
                return TokenType::EqualsEquals;
            return TokenType::Equals;
        })
        TIRO_OP('<', {
            switch (current()) {
                TIRO_OP('=', return TokenType::LessEquals;)
                TIRO_OP('<', return TokenType::LeftShift;)
            default:
                return TokenType::Less;
            }
        })
        TIRO_OP('>', {
            switch (current()) {
                TIRO_OP('=', return TokenType::GreaterEquals;)
                TIRO_OP('>', return TokenType::RightShift;)
            default:
                return TokenType::Greater;
            }
        })
    default:
        return {};

#undef TIRO_OP
    }
}

void Lexer::lex_line_comment() {
    TIRO_DEBUG_ASSERT(current() == '/' && ahead(1) == '/', "Not the start of a line comment.");

    advance(2);
    accept_while([](CodePoint c) { return c != '\n'; });
}

void Lexer::lex_block_comment() {
    TIRO_DEBUG_ASSERT(current() == '/' && ahead(1) == '*', "Not the start of a block comment.");

    size_t depth = 0;
    while (!eof()) {
        CodePoint c = current();
        if (c == '/' && ahead(1) == '*') {
            advance(2);
            ++depth;
        } else if (c == '*' && ahead(1) == '/') {
            TIRO_DEBUG_ASSERT(depth > 0, "Invalid comment depth.");

            advance(2);
            if (--depth == 0) {
                break;
            }
        } else {
            advance();
        }
    }
}

// Possible situations in the following function:
// - In front of the closing quote (-> end of string)
// - In front of an identifier (immediately after a $)
// - In front of a $ or ${ either because they are the front
//   of the string literal or because the string parser paused in front
//   of them in the last run
// - In front of some string content, just parse until one of the situations
//   above is true
TokenType Lexer::lex_string() {
    TIRO_DEBUG_ASSERT(state_.type == String, "Invalid state.");

    if (eof())
        return TokenType::Eof;

    if (accept(state_.string_delim)) {
        pop_state();
        return TokenType::StringEnd;
    }

    if (state_.string_needs_identifier) {
        if (is_identifier_begin(current())) {
            TokenType type = lex_identifier();
            state_.string_needs_identifier = false;
            return type;
        }

        // No valid identifier char after a $, we act as if the string
        // continues normally with content. The parser will emit an error
        // because it expects a valid identifier.
        state_.string_needs_identifier = false;
    }

    if (accept('$')) {
        // Switch back to normal mode parsing for the nested items inside ${ ... }.
        if (accept('{')) {
            push_state();
            state_ = State::normal();
            return TokenType::StringBlockStart;
        }

        state_.string_needs_identifier = true;
        return TokenType::StringVar;
    }

    lex_string_content();
    TIRO_DEBUG_ASSERT(eof() || current() == state_.string_delim || current() == '$',
        "String content must end with one of the delimiters.");
    return TokenType::StringContent;
}

void Lexer::lex_string_content() {
    TIRO_DEBUG_ASSERT(state_.type == String, "Invalid state.");

    while (1) {
        if (eof()) {
            return;
        }

        CodePoint c = current();
        if (c == state_.string_delim || c == '$')
            return;

        if (c == '\\') {
            advance();
            if (eof())
                return;

            advance();
        } else {
            advance();
        }
    }
}

size_t Lexer::pos() const {
    return input_.pos();
}

void Lexer::pos(size_t pos) {
    input_.seek(pos);
}

std::string_view Lexer::value() const {
    const auto start = start_;
    const auto end = pos();
    TIRO_DEBUG_ASSERT(start <= end, "Invalid lexer state, start must be <= end.");
    return file_content_.substr(start, end - start);
}

size_t Lexer::next_pos() const {
    return input_.next_pos();
}

void Lexer::advance() {
    input_.advance();
}

void Lexer::advance(size_t n) {
    input_.advance(n);
}

bool Lexer::eof() const {
    return input_.at_end();
}

CodePoint Lexer::current() const {
    return input_.get();
}

std::optional<CodePoint> Lexer::ahead(size_t n) {
    return input_.peek(n);
}

bool Lexer::accept(CodePoint c) {
    if (input_ && *input_ == c) {
        advance();
        return true;
    }
    return false;
}

std::optional<CodePoint> Lexer::accept_any(std::initializer_list<CodePoint> candidates) {
    if (!eof()) {
        const auto c = current();
        for (const auto v : candidates) {
            if (c == v) {
                advance();
                return c;
            }
        }
    }
    return {};
}

template<typename Func>
void Lexer::accept_while(Func&& func) {
    for (CodePoint c : input_) {
        if (!func(c))
            break;
    }
}

void Lexer::skip_until(CodePoint c) {
    for (CodePoint v : input_) {
        if (v != c)
            break;
    }
}

void Lexer::push_state() {
    states_.push_back(state_);
}

bool Lexer::pop_state() {
    if (!states_.empty()) {
        state_ = std::move(states_.back());
        states_.pop_back();
        return true;
    }
    return false;
}

} // namespace tiro::next
