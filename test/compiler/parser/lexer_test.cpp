#include "compiler/parser/lexer.hpp"

#include "compiler/diagnostics.hpp"

#include <catch.hpp>

using namespace tiro;

namespace {

class TestLexer {
public:
    explicit TestLexer(std::string_view content)
        : file_name_(strings_.insert("unit-test"))
        , lexer_(file_name_, content, strings_, diag_) {}

    StringTable& strings() { return strings_; }
    Diagnostics& diag() { return diag_; }
    Lexer& lexer() { return lexer_; }

    std::string_view value(InternedString str) {
        REQUIRE(str);
        return strings_.value(str);
    }

    Token next(bool allow_error = false) {
        Token tok = lexer_.next();
        if (!allow_error) {
            if (diag_.message_count() > 0) {
                for (const auto& msg : diag_.messages()) {
                    UNSCOPED_INFO(msg.text);
                }
            }

            REQUIRE(diag_.message_count() == 0);
            REQUIRE(!tok.has_error());
        }
        return tok;
    }

    void clear_errors() { diag_ = Diagnostics(); }

    void require_eof() {
        const auto type = next().type();
        CAPTURE(to_token_name(type));
        REQUIRE(type == TokenType::Eof);
    }

private:
    StringTable strings_;
    Diagnostics diag_;
    InternedString file_name_;
    Lexer lexer_;
};

} // namespace

static i64 must_int(const Token& token) {
    REQUIRE(token.data().type() == TokenDataType::Integer);
    return token.data().as_integer();
}

static f64 must_float(const Token& token) {
    REQUIRE(token.data().type() == TokenDataType::Float);
    return token.data().as_float();
}

static InternedString must_string(const Token& token) {
    REQUIRE(token.data().type() == TokenDataType::String);
    return token.data().as_string();
}

TEST_CASE("Lexer should recognize numeric literals", "[lexer]") {
    struct test_t {
        std::string_view source;
        std::variant<i64, double> expected;
    } tests[] = {
        {"123", i64(123)},
        {"123.4", double(123.4)},
        {"0x123", i64(0x123)},
        {"0x123.4", i64(0x123) + 0.25},
        {"0o123", i64(0123)},
        {"0o123.4", i64(0123) + 0.5},
        {"0b01001", i64(9)},
        {"0b01001.0010", i64(9) + 0.125},
        {"123.10101", double(123.10101)},
        {"1___2___3", i64(123)},
        {"1_2_3.4_5", double(123.45)},
        {"1_____.____2____", double(1.2)},
    };

    for (const test_t& test : tests) {
        CAPTURE(test.source);

        TestLexer lex(test.source);

        Token tok = lex.next();
        REQUIRE(tok.source().begin() == 0);
        REQUIRE(tok.source().end() == test.source.size());

        if (std::holds_alternative<i64>(test.expected)) {
            REQUIRE(tok.type() == TokenType::IntegerLiteral);

            i64 value = must_int(tok);
            REQUIRE(value == std::get<i64>(test.expected));
        } else if (std::holds_alternative<double>(test.expected)) {
            REQUIRE(tok.type() == TokenType::FloatLiteral);

            double value = must_float(tok);
            REQUIRE(value == std::get<double>(test.expected));
        } else {
            FAIL();
        }

        lex.require_eof();
    }
}

TEST_CASE(
    "Lexer should return an error when alphabetic "
    "characters are read after a number",
    "[lexer]") {
    std::string_view source = "123aaaa";

    TestLexer lex(source);

    Token tok = lex.next(true);
    REQUIRE(tok.type() == TokenType::IntegerLiteral);
    REQUIRE(tok.has_error());

    REQUIRE(lex.diag().message_count() > 0);
    REQUIRE(lex.diag().has_errors());
}

TEST_CASE("Lexer should recognize string literals", "[lexer]") {
    struct test_t {
        std::string_view source;
        std::string expected;
    } tests[] = {
        {"\"hello world\"", "hello world"},
        {"'hello world'", "hello world"},
        {"'escape \\r\\n'", "escape \r\n"},
        {"\"\\\"\"", "\""},
    };

    const auto verify_static_string = [&](TestLexer& lex, std::string_view source,
                                          std::string_view expected) {
        const auto begin_tok = lex.next();
        REQUIRE((begin_tok.type() == TokenType::SingleQuote
                 || begin_tok.type() == TokenType::DoubleQuote));
        REQUIRE(begin_tok.source().begin() == 0);
        REQUIRE(begin_tok.source().end() == 1);

        lex.lexer().mode(begin_tok.type() == TokenType::SingleQuote ? LexerMode::StringSingleQuote
                                                                    : LexerMode::StringDoubleQuote);

        const auto string_tok = lex.next();
        REQUIRE(string_tok.type() == TokenType::StringContent);
        REQUIRE(string_tok.source().begin() == 1);
        REQUIRE(string_tok.source().end() == source.size() - 1);
        REQUIRE(lex.value(must_string(string_tok)) == expected);

        lex.lexer().mode(LexerMode::Normal);

        const auto end_tok = lex.next();
        REQUIRE(end_tok.type() == begin_tok.type());
        REQUIRE(end_tok.source().begin() == source.size() - 1);
        REQUIRE(end_tok.source().end() == source.size());

        lex.require_eof();
    };

    for (const test_t& test : tests) {
        CAPTURE(test.source);

        TestLexer lex(test.source);
        verify_static_string(lex, test.source, test.expected);
    }
}

TEST_CASE("Lexer should recognize identifiers", "[lexer]") {
    std::string_view source = "a aa a123 a_b_c _1";

    struct expected_t {
        size_t start;
        size_t end;
        std::string name;
    } expected_identifiers[] = {
        {0, 1, "a"}, {2, 4, "aa"}, {5, 9, "a123"}, {10, 15, "a_b_c"}, {16, 18, "_1"}};

    TestLexer lex(source);
    for (const expected_t& expected : expected_identifiers) {
        CAPTURE(expected.name);

        Token tok = lex.next();
        REQUIRE(tok.type() == TokenType::Identifier);
        REQUIRE(tok.source().begin() == expected.start);
        REQUIRE(tok.source().end() == expected.end);
        REQUIRE(lex.value(must_string(tok)) == expected.name);
    }

    lex.require_eof();
}

TEST_CASE("Lexer should recognize symbols", "[lexer]") {
    std::string_view source = "#a123 #456 #__a123";

    struct expected_t {
        size_t start;
        size_t end;
        std::string name;
    } expected_identifiers[] = {{0, 5, "a123"}, {6, 10, "456"}, {11, 18, "__a123"}};

    TestLexer lex(source);
    for (const expected_t& expected : expected_identifiers) {
        CAPTURE(expected.name);

        Token tok = lex.next();
        REQUIRE(tok.type() == TokenType::SymbolLiteral);
        REQUIRE(tok.source().begin() == expected.start);
        REQUIRE(tok.source().end() == expected.end);
        REQUIRE(lex.value(must_string(tok)) == expected.name);
    }

    lex.require_eof();
}

TEST_CASE("Lexer should support unicode identifiers", "[lexer]") {
    std::string_view tests[] = {"normal_identifier_23", "hellöchen", "hello⅞", "世界"};
    for (const auto& source : tests) {
        CAPTURE(source);
        TestLexer lex(source);

        Token tok = lex.next();
        REQUIRE(tok.type() == TokenType::Identifier);
        REQUIRE(tok.source().begin() == 0);
        REQUIRE(tok.source().end() == source.size());
        REQUIRE(lex.value(must_string(tok)) == source);

        lex.require_eof();
    }
}

TEST_CASE("Lexer should identify operators", "[lexer]") {
    std::string_view source =
        "( ) [ ] { } . , : ; ? ?. ?( ?[ ?? + - * ** / % "
        "+= -= *= **= /= %= "
        "++ -- ~ | ^ << >> & ! || && = == != "
        "< > <= >= ' \"";

    TokenType expected_tokens[] = {

        TokenType::LeftParen, TokenType::RightParen, TokenType::LeftBracket,
        TokenType::RightBracket, TokenType::LeftBrace, TokenType::RightBrace, TokenType::Dot,
        TokenType::Comma, TokenType::Colon, TokenType::Semicolon, TokenType::Question,
        TokenType::QuestionDot, TokenType::QuestionLeftParen, TokenType::QuestionLeftBracket,
        TokenType::QuestionQuestion, TokenType::Plus, TokenType::Minus, TokenType::Star,
        TokenType::StarStar, TokenType::Slash, TokenType::Percent,

        TokenType::PlusEquals, TokenType::MinusEquals, TokenType::StarEquals,
        TokenType::StarStarEquals, TokenType::SlashEquals, TokenType::PercentEquals,

        TokenType::PlusPlus, TokenType::MinusMinus, TokenType::BitwiseNot, TokenType::BitwiseOr,
        TokenType::BitwiseXor, TokenType::LeftShift, TokenType::RightShift, TokenType::BitwiseAnd,
        TokenType::LogicalNot, TokenType::LogicalOr, TokenType::LogicalAnd, TokenType::Equals,
        TokenType::EqualsEquals, TokenType::NotEquals,

        TokenType::Less, TokenType::Greater, TokenType::LessEquals, TokenType::GreaterEquals,
        TokenType::SingleQuote, TokenType::DoubleQuote

    };

    TestLexer lex(source);
    for (TokenType expected : expected_tokens) {
        CAPTURE(to_token_name(expected));

        Token tok = lex.next();
        CAPTURE(to_token_name(tok.type()));
        REQUIRE(tok.type() == expected);
    }

    lex.require_eof();
}

TEST_CASE("Lexer should recognize keywords", "[lexer]") {
    std::string_view source =
        "func var const is as in if else while for "
        "continue break switch class struct "
        "protocol true false null import export package "
        "yield async await throw try catch scope Map Set";

    TokenType expected_tokens[] = {
        TokenType::KwFunc,
        TokenType::KwVar,
        TokenType::KwConst,
        TokenType::KwIs,
        TokenType::KwAs,
        TokenType::KwIn,
        TokenType::KwIf,
        TokenType::KwElse,
        TokenType::KwWhile,
        TokenType::KwFor,
        TokenType::KwContinue,
        TokenType::KwBreak,
        TokenType::KwSwitch,
        TokenType::KwClass,
        TokenType::KwStruct,
        TokenType::KwProtocol,
        TokenType::KwTrue,
        TokenType::KwFalse,
        TokenType::KwNull,
        TokenType::KwImport,
        TokenType::KwExport,
        TokenType::KwPackage,
        TokenType::KwYield,
        TokenType::KwAsync,
        TokenType::KwAwait,
        TokenType::KwThrow,
        TokenType::KwTry,
        TokenType::KwCatch,
        TokenType::KwScope,
        TokenType::KwMap,
        TokenType::KwSet,
    };

    TestLexer lex(source);
    for (TokenType expected : expected_tokens) {
        CAPTURE(to_token_name(expected));

        Token tok = lex.next();
        REQUIRE(tok.type() == expected);
        CAPTURE(to_token_name(tok.type()));
    }

    lex.require_eof();
}

TEST_CASE("Lexer should recognize block comments", "[lexer]") {
    std::string_view source = "hello/*world*/;";

    {
        TestLexer lex(source);
        lex.lexer().ignore_comments(true);

        Token tok_ident = lex.next();
        REQUIRE(tok_ident.type() == TokenType::Identifier);
        REQUIRE(lex.value(must_string(tok_ident)) == "hello");

        Token tok_semi = lex.next();
        REQUIRE(tok_semi.type() == TokenType::Semicolon);

        lex.require_eof();
    }

    {
        TestLexer lex(source);
        lex.lexer().ignore_comments(false);

        Token tok_ident = lex.next();
        REQUIRE(tok_ident.type() == TokenType::Identifier);
        REQUIRE(lex.value(must_string(tok_ident)) == "hello");

        Token tok_comment = lex.next();
        REQUIRE(tok_comment.type() == TokenType::Comment);

        size_t begin = tok_comment.source().begin();
        size_t end = tok_comment.source().end();
        REQUIRE(end >= begin);
        REQUIRE(source.substr(begin, end - begin) == "/*world*/");

        Token tok_semi = lex.next();
        REQUIRE(tok_semi.type() == TokenType::Semicolon);

        lex.require_eof();
    }
}

TEST_CASE("Lexer should recognize line comment", "[lexer]") {
    std::string_view source = "asd // + - test;\n [";

    TestLexer lex(source);
    lex.lexer().ignore_comments(false);

    Token tok_ident = lex.next();
    REQUIRE(tok_ident.type() == TokenType::Identifier);
    REQUIRE(lex.value(must_string(tok_ident)) == "asd");

    Token tok_comment = lex.next();
    REQUIRE(tok_comment.type() == TokenType::Comment);

    size_t begin = tok_comment.source().begin();
    size_t end = tok_comment.source().end();
    REQUIRE(end >= begin);
    REQUIRE(source.substr(begin, end - begin) == "// + - test;");

    Token tok_semi = lex.next();
    REQUIRE(tok_semi.type() == TokenType::LeftBracket);

    lex.require_eof();
}

TEST_CASE("Lexer shoulds support nested block comments", "[lexer]") {
    std::string_view source = "   /* 1 /* 2 /* 3 */ 4 */ 5 */   ";

    TestLexer lex(source);
    lex.lexer().ignore_comments(false);

    Token tok_comment = lex.next();
    REQUIRE(tok_comment.type() == TokenType::Comment);

    size_t begin = tok_comment.source().begin();
    size_t end = tok_comment.source().end();
    REQUIRE(end >= begin);
    REQUIRE(source.substr(begin, end - begin) == "/* 1 /* 2 /* 3 */ 4 */ 5 */");

    lex.require_eof();
}

TEST_CASE("Lexer should support interpolated strings", "[lexer]") {
    auto test = [&](std::string_view source, char delim) {
        const char other_delim = delim == '"' ? '\'' : '"';
        const auto delim_type = delim == '"' ? TokenType::DoubleQuote : TokenType::SingleQuote;
        const auto lexer_mode = delim == '"' ? LexerMode::StringDoubleQuote
                                             : LexerMode::StringSingleQuote;

        TestLexer lex(source);

        Token begin = lex.next();
        REQUIRE(begin.type() == delim_type);

        lex.lexer().mode(lexer_mode);

        Token content_1 = lex.next();
        REQUIRE(content_1.type() == TokenType::StringContent);
        REQUIRE(lex.value(must_string(content_1)) == fmt::format("asd{} ", other_delim));

        Token dollar = lex.next();
        REQUIRE(dollar.type() == TokenType::Dollar);

        lex.lexer().mode(LexerMode::Normal);

        Token ident = lex.next();
        REQUIRE(ident.type() == TokenType::Identifier);
        REQUIRE(lex.value(must_string(ident)) == "foo_");

        lex.lexer().mode(lexer_mode);

        Token content_2 = lex.next();
        REQUIRE(content_2.type() == TokenType::StringContent);
        REQUIRE(lex.value(must_string(content_2)) == "$ 123");

        Token end = lex.next();
        REQUIRE(end.type() == delim_type);

        lex.lexer().mode(LexerMode::Normal);
        lex.require_eof();
    };

    std::string_view source_dq = R"(
        "asd' $foo_\$ 123"
    )";
    std::string_view source_sq = R"(
        'asd" $foo_\$ 123'
    )";

    test(source_dq, '"');
    test(source_sq, '\'');
}
