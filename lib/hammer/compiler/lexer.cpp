#include "hammer/compiler/lexer.hpp"

#include "hammer/compiler/diagnostics.hpp"
#include "hammer/core/math.hpp"

namespace hammer {

static constexpr struct {
    std::string_view name;
    TokenType type;
} keywords_table[] = {
    {"func", TokenType::KwFunc},
    {"var", TokenType::KwVar},
    {"const", TokenType::KwConst},
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
    HAMMER_UNREACHABLE("Invalid base.");
}

//static std::string fmt_code_point(code_point cp) {
//    std::string temp;
//    append_code_point(temp, cp);
//    return temp;
//}

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

    if (c == '"' || c == '\'')
        return lex_string();

    if (is_digit(c))
        return lex_number();

    if (is_identifier_begin(c))
        return lex_name();

    if (auto op = lex_operator())
        return std::move(*op);

    std::string cp = code_point_to_string(c);
    SourceReference source = ref(pos());
    diag_.reportf(Diagnostics::Error, source, "Invalid input text: `{}`", cp);
    return Token(TokenType::InvalidToken, source);
}

Token Lexer::lex_string() {
    HAMMER_ASSERT(!input_.at_end(), "Already at the end of file");
    HAMMER_ASSERT(input_.get() == '\"' || input_.get() == '\'',
        "Invalid start for string literals");

    const CodePoint delimiter = input_.get();
    const size_t string_start = pos();
    bool has_error = false;

    input_.advance();
    buffer_.clear();
    while (1) {
        if (input_.at_end()) {
            diag_.report(Diagnostics::Error, ref(string_start),
                "Unterminated string literal at the end of file");
            has_error = true;
            goto end;
        }

        const size_t read_pos = pos();
        const CodePoint read = input_.get();
        if (read == delimiter) {
            input_.advance();
            goto end;
        }

        if (read == '\\') {
            input_.advance();
            if (input_.at_end()) {
                diag_.report(Diagnostics::Error, ref(read_pos, next_pos()),
                    "Incomplete escape sequence");

                has_error = true;
                goto end;
            }

            const CodePoint escape = input_.get();
            CodePoint write = invalid_code_point;
            switch (escape) {
            case 'n':
                write = '\n';
                break;
            case 'r':
                write = '\r';
                break;
            case 't':
                write = '\t';
                break;
            case '"':
            case '\'':
            case '\\':
                write = escape;
                break;

            default: {
                diag_.report(Diagnostics::Error, ref(read_pos, next_pos()),
                    "Invalid escape sequence.");
                has_error = true;
                goto end;
            }
            }

            input_.advance();
            append_code_point(buffer_, write);
        } else {
            input_.advance();
            append_code_point(buffer_, read);
        }
    }

end:
    Token result(TokenType::StringLiteral, ref(string_start));
    result.has_error(has_error);
    result.string_value(strings_.insert(buffer_));

    buffer_.clear();
    return result;
}

Token Lexer::lex_number() {
    HAMMER_ASSERT(!input_.at_end(), "Already at the end of file");
    HAMMER_ASSERT(is_digit(input_.get()), "Code point does not start a number");

    const size_t number_start = pos();

    auto int_token = [&](size_t end, bool has_error, i64 value) {
        Token tok(TokenType::IntegerLiteral, ref(number_start, end));
        tok.has_error(has_error);
        tok.int_value(value);
        return tok;
    };

    auto float_token = [&](size_t end, bool has_error, double value) {
        Token tok(TokenType::FloatLiteral, ref(number_start, end));
        tok.has_error(has_error);
        tok.float_value(value);
        return tok;
    };

    int base = 10; // Real numeric base
    int parse_base =
        10; // More relaxed for parsing [-> error messages for digits]

    // Determine the base of the number literal.
    if (input_.get() == '0') {
        input_.advance();

        const CodePoint base_specifier = input_.get();
        if (is_alpha(base_specifier)) {
            switch (base_specifier) {
            case 'b':
                base = 2;
                break;
            case 'o':
                base = 8;
                break;
            case 'x':
                base = 16;
                parse_base = 16;
                break;
            default: {
                diag_.report(Diagnostics::Error, ref(pos(), next_pos()),
                    "Expected a valid number format specified ('b', 'o' or "
                    "'x').");
                return int_token(pos(), true, 0);
            }
            }

            input_.advance();
        }
    }

    i64 int_value = 0;
    for (CodePoint c : input_) {
        if (c == '_')
            continue;

        if (!to_digit(c, parse_base))
            break;

        if (auto digit = to_digit(c, base)) {
            if (!checked_mul<i64>(int_value, base, int_value)
                || !checked_add<i64>(int_value, *digit, int_value)) {
                diag_.report(Diagnostics::Error, ref(number_start, next_pos()),
                    "Number is too large (overflow)");
                // TODO skip other numbers?
                return int_token(next_pos(), true, 0);
            }
        } else {
            diag_.reportf(Diagnostics::Error, ref(pos(), next_pos()),
                "Invalid digit for base {} number", base);
            return int_token(pos(), true, int_value);
        }
    }

    skip('_');
    if (input_.at_end()) {
        return int_token(pos(), false, int_value);
    }

    if (input_.get() == '.') {
        input_.advance();

        const double base_inv = 1.0 / base;
        double float_value = 0;
        double pow = base_inv;

        for (CodePoint c : input_) {
            if (c == '_')
                continue;

            if (!to_digit(c, parse_base))
                break;

            if (auto digit = to_digit(c, base)) {
                float_value += *digit * pow;
                pow *= base_inv;
            } else {
                diag_.reportf(Diagnostics::Error, ref(pos(), next_pos()),
                    "Invalid digit for base {} number", base);
                return float_token(
                    pos(), true, static_cast<double>(int_value) + float_value);
            }
        }
        skip('_');

        // TODO: bad float parsing
        Token result = float_token(
            pos(), false, static_cast<double>(int_value) + float_value);
        if (!input_.at_end() && is_identifier_part(input_.get())) {
            result.has_error(true);
            diag_.report(Diagnostics::Error, ref(pos(), next_pos()),
                "Invalid alphabetic character after number");
        }
        return result;
    }

    Token result = int_token(pos(), false, int_value);
    if (!input_.at_end() && is_identifier_part(input_.get())) {
        result.has_error(true);
        diag_.report(Diagnostics::Error, ref(pos(), next_pos()),
            "Invalid alphabetic character after number");
    }
    return result;
}

Token Lexer::lex_name() {
    HAMMER_ASSERT(!input_.at_end(), "Already at the end of file");
    HAMMER_ASSERT(is_identifier_begin(input_.get()),
        "Code point does not start an identifier.");

    const size_t name_start = pos();
    for (CodePoint c : input_) {
        if (!is_identifier_part(c))
            break;
    }

    InternedString string = strings_.insert(
        file_content_.substr(name_start, input_.pos() - name_start));

    TokenType type = TokenType::Identifier;
    if (auto kw_pos = keywords_.find(string); kw_pos != keywords_.end()) {
        type = kw_pos->second;
    }

    Token tok(type, ref(name_start));
    tok.string_value(string);
    return tok;
}

std::optional<Token> Lexer::lex_operator() {
    HAMMER_ASSERT(!input_.at_end(), "Already at the end of file");

    const size_t begin = pos();

    CodePointRange& p = input_;
    CodePoint c = p.get();
    auto getop = [&]() -> std::optional<TokenType> {
        switch (c) {

        // Braces
        case '(':
            ++p;
            return TokenType::LParen;
        case ')':
            ++p;
            return TokenType::RParen;
        case '[':
            ++p;
            return TokenType::LBracket;
        case ']':
            ++p;
            return TokenType::RBracket;
        case '{':
            ++p;
            return TokenType::LBrace;
        case '}':
            ++p;
            return TokenType::RBrace;

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
            return TokenType::Plus;
        }
        case '-': {
            ++p;
            if (p.current() == '-') {
                ++p;
                return TokenType::MinusMinus;
            }
            return TokenType::Minus;
        }
        case '*': {
            ++p;
            if (p.current() == '*') {
                ++p;
                return TokenType::Starstar;
            }
            return TokenType::Star;
        }
        case '/':
            ++p;
            return TokenType::Slash;
        case '%':
            ++p;
            return TokenType::Percent;
        case '~':
            ++p;
            return TokenType::BNot;
        case '^':
            ++p;
            return TokenType::BXor;
        case '!': {
            ++p;
            if (p.current() == '=') {
                ++p;
                return TokenType::NEq;
            }
            return TokenType::LNot;
        }
        case '|': {
            ++p;
            if (p.current() == '|') {
                ++p;
                return TokenType::LOr;
            }
            return TokenType::BOr;
        }
        case '&': {
            ++p;
            if (p.current() == '&') {
                ++p;
                return TokenType::LAnd;
            }
            return TokenType::BAnd;
        }
        case '=': {
            ++p;
            if (p.current() == '=') {
                ++p;
                return TokenType::EqEq;
            }
            return TokenType::Eq;
        }
        case '<': {
            ++p;
            if (p.current() == '=') {
                ++p;
                return TokenType::LessEq;
            }
            return TokenType::Less;
        }
        case '>': {
            ++p;
            if (p.current() == '=') {
                ++p;
                return TokenType::GreaterEq;
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
    HAMMER_ASSERT(input_.current() == '/' && input_.peek() == '/',
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
    HAMMER_ASSERT(input_.current() == '/' && input_.peek() == '*',
        "Not the start of a block comment.");

    const size_t begin = pos();

    size_t depth = 0;
    while (!input_.at_end()) {
        CodePoint c = input_.get();
        if (c == '/' && input_.peek() == '*') {
            input_.advance(2);
            ++depth;
        } else if (c == '*' && input_.peek() == '/') {
            HAMMER_ASSERT(depth > 0, "Invalid comment depth.");

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

void Lexer::skip(CodePoint c) {
    for (CodePoint v : input_) {
        if (v != c)
            break;
    }
}

} // namespace hammer
