#include "tiro/syntax/lexer.hpp"

#include "tiro/compiler/diagnostics.hpp"
#include "tiro/core/math.hpp"
#include "tiro/core/range_utils.hpp"
#include "tiro/core/safe_int.hpp"

namespace tiro::compiler {

static constexpr struct {
    std::string_view name;
    TokenType type;
} keywords_table[] = {
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
    {"Map", TokenType::KwMap},
    {"Set", TokenType::KwSet},
    {"yield", TokenType::KwYield},
    {"async", TokenType::KwAsync},
    {"await", TokenType::KwAwait},
    {"throw", TokenType::KwThrow},
    {"try", TokenType::KwTry},
    {"catch", TokenType::KwCatch},
    {"scope", TokenType::KwScope},
};

// Attempts to parse the given code point as a digit with the given base.
static std::optional<int> to_digit(CodePoint c, int base) {
    switch (base) {
    case 2: {
        if (c >= '0' && c <= '1')
            return c - '0';
        return {};
    }
    case 8: {
        if (c >= '0' && c <= '7') {
            return c - '0';
        }
        return {};
    }
    case 10: {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        return {};
    }
    case 16: {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        return {};
    }
    }
    TIRO_UNREACHABLE("Invalid base.");
}

static bool is_decimal_digit(CodePoint c) {
    return c >= '0' && c <= '9';
}

static bool is_identifier_begin(CodePoint c) {
    return is_letter(c) || c == '_';
}

static bool is_identifier_part(CodePoint c) {
    return is_identifier_begin(c) || is_number(c);
}

Lexer::Lexer(InternedString file_name, std::string_view file_content,
    StringTable& strings, Diagnostics& diag)
    : strings_(strings)
    , file_name_(file_name)
    , file_content_(file_content)
    , diag_(diag)
    , input_(file_content) {

    for (const auto& e : keywords_table) {
        keywords_.emplace(strings_.insert(e.name), e.type);
    }
}

Token Lexer::next() {
    if (mode_ == LexerMode::StringSingleQuote
        || mode_ == LexerMode::StringDoubleQuote)
        return lex_string_literal();

again:
    // Skip whitespace
    for (CodePoint c : input_) {
        if (!is_whitespace(c))
            break;
    }

    if (input_.at_end())
        return Token(TokenType::Eof, ref(pos()));

    CodePoint c = input_.get();

    if (c == '/' && input_.peek() == '/') {
        Token tok = lex_line_comment();
        if (ignore_comments_)
            goto again;
        return tok;
    }

    if (c == '/' && input_.peek() == '*') {
        Token tok = lex_block_comment();
        if (ignore_comments_)
            goto again;
        return tok;
    }

    if (c == '\'' || c == '"') {
        const auto p = pos();
        const auto type = c == '"' ? TokenType::DoubleQuote
                                   : TokenType::SingleQuote;
        input_.advance();
        return Token(type, ref(p));
    }

    if (is_decimal_digit(c))
        return mode_ == LexerMode::Member ? lex_numeric_member() : lex_number();

    if (c == '#')
        return lex_symbol();

    if (is_identifier_begin(c))
        return lex_name();

    if (auto op = lex_operator())
        return std::move(*op);

    std::string cp = to_string_utf8(c);
    SourceReference source = ref(pos());
    diag_.reportf(Diagnostics::Error, source, "Invalid input text: `{}`", cp);
    return Token(TokenType::InvalidToken, source);
}

// Possible situations in the following function:
// - In front of the closing quote (-> end of string)
// - In front of a $ or ${ either because they are the front
//   of the string literal or because the string parser paused in front
//   of them in the last run
// - In front of some string content, just parse until one of the situations
//   above is true
Token Lexer::lex_string_literal() {
    TIRO_ASSERT(mode_ == LexerMode::StringSingleQuote
                    || mode_ == LexerMode::StringDoubleQuote,
        "Must not be called without valid lexer mode.");

    const CodePoint delim = mode_ == LexerMode::StringSingleQuote ? '\'' : '"';
    const size_t begin = pos();

    if (input_.at_end())
        return Token(TokenType::Eof, ref(begin));

    if (input_.get() == delim) {
        input_.advance();
        const auto type = mode_ == LexerMode::StringSingleQuote
                              ? TokenType::SingleQuote
                              : TokenType::DoubleQuote;
        return Token(type, ref(begin));
    }

    if (input_.get() == '$') {
        TokenType type = TokenType::Dollar;
        input_.advance();
        if (input_.current() == '{') {
            type = TokenType::DollarLeftBrace;
            input_.advance();
        }
        return Token(type, ref(begin));
    }

    buffer_.clear();
    bool ok = lex_string_content(begin, {'$', delim}, buffer_);
    if (ok) {
        // The delimiter is not part of the returned content - it will be produced by the next call.
        TIRO_ASSERT(input_.get() == delim || input_.get() == '$',
            "Successful string content must end with one of the delimiters.");
    }

    Token result(TokenType::StringContent, ref(begin));
    result.has_error(!ok);
    result.string_value(strings_.insert(buffer_));

    buffer_.clear();
    return result;
}

Token Lexer::lex_number() {
    TIRO_ASSERT(!input_.at_end(), "Already at the end of file.");
    TIRO_ASSERT(
        is_decimal_digit(input_.get()), "Code point does not start a number");

    const size_t number_start = pos();

    auto int_token = [&](size_t end, bool has_error, i64 value) {
        Token tok(TokenType::IntegerLiteral, ref(number_start, end));
        tok.has_error(has_error);
        tok.int_value(value);
        return tok;
    };

    auto float_token = [&](size_t end, bool has_error, f64 value) {
        Token tok(TokenType::FloatLiteral, ref(number_start, end));
        tok.has_error(has_error);
        tok.float_value(value);
        return tok;
    };

    // Real numeric base for string -> numeric value conversion
    int base = 10;

    // More relaxed base for parsing [-> error messages for digits]
    int parse_base = 10;

    // Determine the base of the number literal.
    if (input_.get() == '0') {
        input_.advance();

        const CodePoint base_specifier = input_.get();
        switch (base_specifier) {
        case 'b':
            base = 2;
            input_.advance();
            break;
        case 'o':
            base = 8;
            input_.advance();
            break;
        case 'x':
            base = 16;
            parse_base = 16;
            input_.advance();
            break;
        default: {
            if (is_letter(base_specifier)) {
                diag_.report(Diagnostics::Error, ref(pos(), next_pos()),
                    "Expected a digit or a valid number format specifier ('b', "
                    "'o' or 'x').");
                return int_token(pos(), true, 0);
            }
            break;
        }
        }
    }

    // TODO Skip remaining numbers (with an error) on overflow to recover.

    // Parse the integer part of the number literal
    i64 int_value = 0;
    {
        SafeInt<i64> safe_int = 0;
        for (CodePoint c : input_) {
            if (c == '_')
                continue;

            if (!to_digit(c, parse_base))
                break;

            if (auto digit = to_digit(c, base); TIRO_LIKELY(digit)) {
                if (!safe_int.try_mul(base) || !safe_int.try_add(*digit)) {
                    diag_.report(Diagnostics::Error,
                        ref(number_start, next_pos()),
                        "Number is too large (overflow).");
                    return int_token(next_pos(), true, 0);
                }
            } else {
                diag_.reportf(Diagnostics::Error, ref(pos(), next_pos()),
                    "Invalid digit for base {} number.", base);
                return int_token(pos(), true, safe_int.value());
            }
        }
        int_value = safe_int.value();
    }

    skip('_');
    if (input_.at_end()) {
        return int_token(pos(), false, int_value);
    }

    // Parse an optional fractional part
    if (input_.get() == '.') {
        input_.advance();

        const f64 base_inv = 1.0 / base;
        f64 float_value = 0;
        f64 pow = base_inv;

        for (CodePoint c : input_) {
            if (c == '_')
                continue;

            if (!to_digit(c, parse_base))
                break;

            if (auto digit = to_digit(c, base); TIRO_LIKELY(digit)) {
                float_value += *digit * pow;
                pow *= base_inv;
            } else {
                diag_.reportf(Diagnostics::Error, ref(pos(), next_pos()),
                    "Invalid digit for base {} number.", base);
                return float_token(
                    pos(), true, static_cast<f64>(int_value) + float_value);
            }
        }
        skip('_');

        // TODO: bad float parsing
        Token result = float_token(
            pos(), false, static_cast<f64>(int_value) + float_value);
        if (!input_.at_end() && is_identifier_part(input_.get())) {
            result.has_error(true);
            diag_.report(Diagnostics::Error, ref(pos(), next_pos()),
                "Invalid start of an identifier after a number.");
        }
        return result;
    }

    Token result = int_token(pos(), false, int_value);
    if (!input_.at_end() && is_identifier_part(input_.get())) {
        result.has_error(true);
        diag_.report(Diagnostics::Error, ref(pos(), next_pos()),
            "Invalid start of an identifier after a number.");
    }
    return result;
}

Token Lexer::lex_numeric_member() {
    TIRO_ASSERT(!input_.at_end(), "Already at the end of file.");
    TIRO_ASSERT(
        is_decimal_digit(input_.get()), "Code point does not start a number");

    const size_t number_start = pos();

    auto token = [&](size_t end, bool has_error, i64 value) {
        Token tok(TokenType::NumericMember, ref(number_start, end));
        tok.has_error(has_error);
        tok.int_value(value);
        return tok;
    };

    SafeInt<i64> value;
    for (CodePoint c : input_) {
        if (!to_digit(c, 16))
            break;

        auto digit = to_digit(c, 10);
        if (TIRO_UNLIKELY(!digit)) {
            diag_.report(Diagnostics::Error, ref(pos(), next_pos()),
                "Only decimal digits are permitted for numeric members.");
            return token(pos(), true, 0);
        }

        if (!value.try_mul(10) || !value.try_add(*digit)) {
            diag_.report(Diagnostics::Error, ref(number_start, next_pos()),
                "Number is too large (overflow).");
            return token(next_pos(), true, 0);
        }
    }

    const size_t number_end = pos();

    Token result = token(number_end, false, value.value());
    std::string_view str_value = substr(number_start, number_end);
    if (!str_value.empty() && str_value[0] == '0' && str_value != "0") {
        result.has_error(true);
        diag_.report(Diagnostics::Error, ref(number_start, number_end),
            "Leading zeroes are forbidden for numeric members.");
    }

    if (!input_.at_end() && is_identifier_part(input_.get())) {
        result.has_error(true);
        diag_.report(Diagnostics::Error, ref(pos(), next_pos()),
            "Invalid start of an identifier after a numeric member.");
    }

    return result;
}

Token Lexer::lex_name() {
    TIRO_ASSERT(!input_.at_end(), "Already at the end of file.");
    TIRO_ASSERT(is_identifier_begin(input_.get()),
        "Code point does not start an identifier.");

    const size_t name_start = pos();
    for (CodePoint c : input_) {
        if (!is_identifier_part(c))
            break;
    }

    InternedString string = strings_.insert(substr(name_start, input_.pos()));

    TokenType type = TokenType::Identifier;
    if (auto kw_pos = keywords_.find(string); kw_pos != keywords_.end()) {
        type = kw_pos->second;
    }

    Token tok(type, ref(name_start));
    tok.string_value(string);
    return tok;
}

Token Lexer::lex_symbol() {
    TIRO_ASSERT(!input_.at_end(), "Already at the end of file.");
    TIRO_ASSERT(input_.get() == '#', "Symbols must start with #.");

    const size_t sym_start = pos();
    input_.advance(); // skip #

    const size_t string_start = pos();
    for (CodePoint c : input_) {
        if (!is_identifier_part(c))
            break;
    }
    const size_t string_end = pos();

    InternedString string = strings_.insert(substr(string_start, string_end));

    Token tok(TokenType::SymbolLiteral, ref(sym_start));
    if (string_start == string_end) {
        diag_.report(Diagnostics::Error, tok.source(),
            "Empty symbol literals are not allowed.");
        tok.has_error(true);
    }
    tok.string_value(string);
    return tok;
}

std::optional<Token> Lexer::lex_operator() {
    TIRO_ASSERT(!input_.at_end(), "Already at the end of file.");

    const size_t begin = pos();

    CodePointRange& p = input_;
    CodePoint c = p.get();

    auto getop = [&]() -> std::optional<TokenType> {
        switch (c) {

        // Braces
        case '(':
            ++p;
            return TokenType::LeftParen;
        case ')':
            ++p;
            return TokenType::RightParen;
        case '[':
            ++p;
            return TokenType::LeftBracket;
        case ']':
            ++p;
            return TokenType::RightBracket;
        case '{':
            ++p;
            return TokenType::LeftBrace;
        case '}':
            ++p;
            return TokenType::RightBrace;

        // Operators
        case '.':
            ++p;
            return TokenType::Dot;
        case ',':
            ++p;
            return TokenType::Comma;
        case ':':
            ++p;
            return TokenType::Colon;
        case ';':
            ++p;
            return TokenType::Semicolon;
        case '?':
            ++p;
            return TokenType::Question;
        case '+': {
            ++p;
            if (p.current() == '+') {
                ++p;
                return TokenType::PlusPlus;
            }
            if (p.current() == '=') {
                ++p;
                return TokenType::PlusEquals;
            }
            return TokenType::Plus;
        }
        case '-': {
            ++p;
            if (p.current() == '-') {
                ++p;
                return TokenType::MinusMinus;
            }
            if (p.current() == '=') {
                ++p;
                return TokenType::MinusEquals;
            }
            return TokenType::Minus;
        }
        case '*': {
            ++p;
            if (p.current() == '*') {
                ++p;
                if (p.current() == '=') {
                    ++p;
                    return TokenType::StarStarEquals;
                }
                return TokenType::StarStar;
            }
            if (p.current() == '=') {
                ++p;
                return TokenType::StarEquals;
            }
            return TokenType::Star;
        }
        case '/': {
            ++p;
            if (p.current() == '=') {
                ++p;
                return TokenType::SlashEquals;
            }
            return TokenType::Slash;
        }
        case '%': {
            ++p;
            if (p.current() == '=') {
                ++p;
                return TokenType::PercentEquals;
            }
            return TokenType::Percent;
        }
        case '~':
            ++p;
            return TokenType::BitwiseNot;
        case '^':
            ++p;
            return TokenType::BitwiseXor;
        case '!': {
            ++p;
            if (p.current() == '=') {
                ++p;
                return TokenType::NotEquals;
            }
            return TokenType::LogicalNot;
        }
        case '|': {
            ++p;
            if (p.current() == '|') {
                ++p;
                return TokenType::LogicalOr;
            }
            return TokenType::BitwiseOr;
        }
        case '&': {
            ++p;
            if (p.current() == '&') {
                ++p;
                return TokenType::LogicalAnd;
            }
            return TokenType::BitwiseAnd;
        }
        case '=': {
            ++p;
            if (p.current() == '=') {
                ++p;
                return TokenType::EqualsEquals;
            }
            return TokenType::Equals;
        }
        case '<': {
            ++p;
            if (p.current() == '=') {
                ++p;
                return TokenType::LessEquals;
            }
            if (p.current() == '<') {
                ++p;
                return TokenType::LeftShift;
            }
            return TokenType::Less;
        }
        case '>': {
            ++p;
            if (p.current() == '=') {
                ++p;
                return TokenType::GreaterEquals;
            }
            if (p.current() == '>') {
                ++p;
                return TokenType::RightShift;
            }
            return TokenType::Greater;
        }
        default:
            return {};
        }
    };

    if (auto op = getop()) {
        return Token(*op, ref(begin));
    }
    return {};
}

Token Lexer::lex_line_comment() {
    TIRO_ASSERT(input_.current() == '/' && input_.peek() == '/',
        "Not the start of a line comment.");

    const size_t begin = pos();

    input_.advance(2);
    for (CodePoint c : input_) {
        if (c == '\n')
            break;
    }

    return Token(TokenType::Comment, ref(begin));
}

Token Lexer::lex_block_comment() {
    TIRO_ASSERT(input_.current() == '/' && input_.peek() == '*',
        "Not the start of a block comment.");

    const size_t begin = pos();

    size_t depth = 0;
    while (!input_.at_end()) {
        CodePoint c = input_.get();
        if (c == '/' && input_.peek() == '*') {
            input_.advance(2);
            ++depth;
        } else if (c == '*' && input_.peek() == '/') {
            TIRO_ASSERT(depth > 0, "Invalid comment depth.");

            input_.advance(2);
            if (--depth == 0) {
                break;
            }
        } else {
            input_.advance();
        }
    }

    return Token(TokenType::Comment, ref(begin));
}

bool Lexer::lex_string_content(size_t string_start,
    std::initializer_list<CodePoint> delim, std::string& buffer) {

    while (1) {
        if (input_.at_end()) {
            diag_.report(Diagnostics::Error, ref(string_start),
                "Unterminated string literal at the end of file.");
            return false;
        }

        const size_t read_pos = pos();
        const CodePoint read = input_.get();
        if (contains(delim, read)) {
            return true;
        }

        if (read == '\\') {
            input_.advance();
            if (input_.at_end()) {
                diag_.report(Diagnostics::Error, ref(read_pos, next_pos()),
                    "Incomplete escape sequence.");
                return false;
            }

            const CodePoint escape_char = input_.get();
            CodePoint escape_result = invalid_code_point;
            switch (escape_char) {
            case 'n':
                escape_result = '\n';
                break;
            case 'r':
                escape_result = '\r';
                break;
            case 't':
                escape_result = '\t';
                break;
            case '"':
            case '\'':
            case '\\':
            case '$':
                escape_result = escape_char;
                break;

            default: {
                diag_.report(Diagnostics::Error, ref(read_pos, next_pos()),
                    "Invalid escape sequence.");
                return false;
            }
            }

            input_.advance();
            append_utf8(buffer, escape_result);
        } else {
            input_.advance();
            append_utf8(buffer, read);
        }
    }
}

size_t Lexer::pos() const {
    return input_.pos();
}

size_t Lexer::next_pos() const {
    return input_.next_pos();
}

SourceReference Lexer::ref(size_t begin) const {
    return ref(begin, pos());
}

SourceReference Lexer::ref(size_t begin, size_t end) const {
    return SourceReference::from_std_offsets(file_name_, begin, end);
}

std::string_view Lexer::substr(size_t begin, size_t end) const {
    TIRO_ASSERT(begin <= end, "Invalid offsets: end must be >= begin.");
    TIRO_ASSERT(end <= file_content_.size(), "Offsets out of bounds.");
    return file_content_.substr(begin, end - begin);
}

void Lexer::skip(CodePoint c) {
    for (CodePoint v : input_) {
        if (v != c)
            break;
    }
}

} // namespace tiro::compiler
