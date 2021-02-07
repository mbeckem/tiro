#include "compiler/syntax/lexer.hpp"

#include <catch2/catch.hpp>

using namespace tiro;
using namespace tiro::next;

namespace {

template<typename T>
static std::string format(const T& value) {
    StringFormatStream stream;
    stream.format("{}", value);
    return stream.take_str();
}

class TestLexer {
public:
    explicit TestLexer(std::string_view content)
        : content_(content)
        , lexer_(content) {}

    Lexer& lexer() { return lexer_; }

    Token next() { return lexer_.next(); }

    Token require_next(TokenType expected) {
        const auto token = next();
        require_type(token, expected);
        return token;
    }

    Token require_eof() { return require_next(TokenType::Eof); }

    void require_type(const Token& token, const TokenType& expected_type) {
        const auto actual_type = token.type();
        const auto content = substring(content_, token.source());
        CAPTURE(content);
        CAPTURE(format(actual_type));
        CAPTURE(format(expected_type));
        REQUIRE(actual_type == expected_type);
    }

    void require_range(const Token& token, const SourceRange& expected_range) {
        const auto actual_range = token.source();
        CAPTURE(format(actual_range));
        CAPTURE(format(expected_range));
        REQUIRE((actual_range == expected_range));
    }

    void require_content(const Token& token, std::string_view expected_content) {
        const auto actual_content = substring(content_, token.source());
        REQUIRE(actual_content == expected_content);
    }

    void require_sequence(std::initializer_list<std::tuple<TokenType, std::string_view>> seq) {
        for (const auto& [type, content] : seq) {
            auto token = require_next(type);
            require_content(token, content);
        }
    }

private:
    std::string_view content_;
    Lexer lexer_;
};

} // namespace

TEST_CASE("New Lexer should recognize numeric literals", "[lexer]") {
    // TODO: lexer does not parse the value anymore
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
        lex.require_range(tok, SourceRange(0, test.source.size()));

        if (std::holds_alternative<i64>(test.expected)) {
            lex.require_type(tok, TokenType::Integer);
        } else if (std::holds_alternative<double>(test.expected)) {
            lex.require_type(tok, TokenType::Float);
        } else {
            FAIL();
        }

        lex.require_eof();
    }
}

TEST_CASE("New lexer should not error for unbalanced braces", "[lexer]") {
    std::string_view source = "}}}";

    TestLexer lex(source);
    lex.require_sequence({
        {TokenType::RightBrace, "}"},
        {TokenType::RightBrace, "}"},
        {TokenType::RightBrace, "}"},
    });
    lex.require_eof();
}

TEST_CASE("New Lexer should return allow alphabetic character after a number", "[lexer]") {
    std::string_view source = "123aaaa";

    TestLexer lex(source);

    auto integer = lex.require_next(TokenType::Integer);
    lex.require_content(integer, "123");

    auto identifier = lex.require_next(TokenType::Identifier);
    lex.require_content(identifier, "aaaa");

    lex.require_eof();
}

TEST_CASE("New Lexer should recognize string literals", "[lexer]") {
    struct test_t {
        std::string_view source;
    } tests[] = {
        {"\"hello world\""},
        {"'hello world'"},
        {"'escape \\r\\n'"},
        {"\"\\\"\""},
    };

    for (const test_t& test : tests) {
        const auto source = test.source;
        CAPTURE(source);

        TestLexer lex(test.source);

        const auto begin_tok = lex.require_next(TokenType::StringStart);
        lex.require_content(begin_tok, source.substr(0, 1));
        lex.require_range(begin_tok, {0, 1});

        const auto string_tok = lex.require_next(TokenType::StringContent);
        lex.require_content(string_tok, source.substr(1, source.size() - 2));
        lex.require_range(string_tok, SourceRange(1, source.size() - 1));

        const auto end_tok = lex.require_next(TokenType::StringEnd);
        lex.require_content(end_tok, source.substr(0, 1));
        lex.require_range(end_tok, SourceRange(source.size() - 1, source.size()));

        lex.require_eof();
    }
}

TEST_CASE("New Lexer should recognize identifiers", "[lexer]") {
    std::string_view source = "a aa a123 a_b_c _1";

    struct expected_t {
        size_t start;
        size_t end;
        std::string name;
    } expected_identifiers[] = {
        {0, 1, "a"},
        {2, 4, "aa"},
        {5, 9, "a123"},
        {10, 15, "a_b_c"},
        {16, 18, "_1"},
    };

    TestLexer lex(source);
    for (const expected_t& expected : expected_identifiers) {
        CAPTURE(expected.name);

        Token tok = lex.require_next(TokenType::Identifier);
        lex.require_range(tok, SourceRange(expected.start, expected.end));
        lex.require_content(tok, expected.name);
    }

    lex.require_eof();
}

TEST_CASE("New Lexer should recognize symbols", "[lexer]") {
    std::string_view source = "#a123 #red #__a123";

    struct expected_t {
        size_t start;
        size_t end;
        std::string name;
    } expected_identifiers[] = {
        {0, 5, "#a123"},
        {6, 10, "#red"},
        {11, 18, "#__a123"},
    };

    TestLexer lex(source);
    for (const expected_t& expected : expected_identifiers) {
        CAPTURE(expected.name);

        Token tok = lex.require_next(TokenType::Symbol);
        lex.require_range(tok, SourceRange(expected.start, expected.end));
        lex.require_content(tok, expected.name);
    }

    lex.require_eof();
}

TEST_CASE("New Lexer should support unicode identifiers", "[lexer]") {
    std::string_view tests[] = {"normal_identifier_23", "hellöchen", "hello⅞", "世界"};
    for (const auto& source : tests) {
        CAPTURE(source);
        TestLexer lex(source);

        Token tok = lex.require_next(TokenType::Identifier);
        lex.require_range(tok, SourceRange(0, source.size()));
        lex.require_content(tok, source);

        lex.require_eof();
    }
}

TEST_CASE("New Lexer should identify operators", "[lexer]") {
    std::string_view source =
        "( ) [ ] { } map{ set{ . , : ; ? ?. ?( ?[ ?? + - * ** / % "
        "+= -= *= **= /= %= "
        "++ -- ~ | ^ << >> & ! || && = == != "
        "< > <= >=";

    TokenType expected_tokens[] = {
        TokenType::LeftParen,
        TokenType::RightParen,
        TokenType::LeftBracket,
        TokenType::RightBracket,
        TokenType::LeftBrace,
        TokenType::RightBrace,
        TokenType::MapStart,
        TokenType::SetStart,
        TokenType::Dot,
        TokenType::Comma,
        TokenType::Colon,
        TokenType::Semicolon,
        TokenType::Question,
        TokenType::QuestionDot,
        TokenType::QuestionLeftParen,
        TokenType::QuestionLeftBracket,
        TokenType::QuestionQuestion,
        TokenType::Plus,
        TokenType::Minus,
        TokenType::Star,
        TokenType::StarStar,
        TokenType::Slash,
        TokenType::Percent,

        TokenType::PlusEquals,
        TokenType::MinusEquals,
        TokenType::StarEquals,
        TokenType::StarStarEquals,
        TokenType::SlashEquals,
        TokenType::PercentEquals,

        TokenType::PlusPlus,
        TokenType::MinusMinus,
        TokenType::BitwiseNot,
        TokenType::BitwiseOr,
        TokenType::BitwiseXor,
        TokenType::LeftShift,
        TokenType::RightShift,
        TokenType::BitwiseAnd,
        TokenType::LogicalNot,
        TokenType::LogicalOr,
        TokenType::LogicalAnd,
        TokenType::Equals,
        TokenType::EqualsEquals,
        TokenType::NotEquals,

        TokenType::Less,
        TokenType::Greater,
        TokenType::LessEquals,
        TokenType::GreaterEquals,
    };

    TestLexer lex(source);
    for (TokenType expected : expected_tokens) {
        lex.require_next(expected);
    }
    lex.require_eof();
}

TEST_CASE("New Lexer should recognize keywords", "[lexer]") {
    std::string_view source =
        "func var const is as in if else while for "
        "continue break switch class struct "
        "protocol true false null import export package "
        "yield async await throw try catch scope defer";

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
        TokenType::KwDefer,
    };

    TestLexer lex(source);
    for (TokenType expected : expected_tokens) {
        lex.require_next(expected);
    }
    lex.require_eof();
}

TEST_CASE("New Lexer should recognize block comments", "[lexer]") {
    std::string_view source = "hello/*world*/;";

    {
        TestLexer lex(source);
        lex.lexer().ignore_comments(true);

        Token tok_ident = lex.require_next(TokenType::Identifier);
        lex.require_content(tok_ident, "hello");

        Token tok_semi = lex.require_next(TokenType::Semicolon);
        lex.require_content(tok_semi, ";");

        lex.require_eof();
    }

    {
        TestLexer lex(source);
        lex.lexer().ignore_comments(false);

        Token tok_ident = lex.require_next(TokenType::Identifier);
        lex.require_content(tok_ident, "hello");

        Token tok_comment = lex.require_next(TokenType::Comment);
        lex.require_content(tok_comment, "/*world*/");

        Token tok_semi = lex.require_next(TokenType::Semicolon);
        lex.require_content(tok_semi, ";");

        lex.require_eof();
    }
}

TEST_CASE("New Lexer should recognize line comment", "[lexer]") {
    std::string_view source = "asd // + - test;\n [";

    TestLexer lex(source);
    lex.lexer().ignore_comments(false);

    Token tok_ident = lex.require_next(TokenType::Identifier);
    lex.require_content(tok_ident, "asd");

    Token tok_comment = lex.require_next(TokenType::Comment);
    lex.require_content(tok_comment, "// + - test;");

    Token tok_bracket = lex.require_next(TokenType::LeftBracket);
    lex.require_content(tok_bracket, "[");

    lex.require_eof();
}

TEST_CASE("New Lexer should support nested block comments", "[lexer]") {
    std::string_view source = "   /* 1 /* 2 /* 3 */ 4 */ 5 */   ";

    TestLexer lex(source);
    lex.lexer().ignore_comments(false);

    Token tok_comment = lex.require_next(TokenType::Comment);
    lex.require_content(tok_comment, "/* 1 /* 2 /* 3 */ 4 */ 5 */");

    lex.require_eof();
}

TEST_CASE("New Lexer should support interpolated strings", "[lexer]") {
    auto test = [&](std::string_view source, char delim, char other_delim) {
        TestLexer lex(source);

        Token begin = lex.require_next(TokenType::StringStart);
        lex.require_content(begin, fmt::format("{}", delim));

        Token content_1 = lex.require_next(TokenType::StringContent);
        lex.require_content(content_1, fmt::format("asd{} ", other_delim));

        Token dollar = lex.require_next(TokenType::StringVar);
        lex.require_content(dollar, "$");

        Token ident = lex.require_next(TokenType::Identifier);
        lex.require_content(ident, "foo_");

        Token content_2 = lex.require_next(TokenType::StringContent);
        lex.require_content(content_2, "\\$ 123");

        Token end = lex.require_next(TokenType::StringEnd);
        lex.require_content(end, fmt::format("{}", delim));
        lex.require_eof();
    };

    std::string_view source_dq = R"(
        "asd' $foo_\$ 123"
    )";
    std::string_view source_sq = R"(
        'asd" $foo_\$ 123'
    )";

    test(source_dq, '"', '\'');
    test(source_sq, '\'', '"');
}

TEST_CASE("New lexer should support interpolated strings with expression blocks", "[lexer]") {
    TestLexer lex(R"(
        "hello ${name ?? {"world";} + 1}}}!"
    )");

    lex.require_sequence({
        {TokenType::StringStart, "\""},
        {TokenType::StringContent, "hello "},
        {TokenType::StringBlockStart, "${"},
        {TokenType::Identifier, "name"},
        {TokenType::QuestionQuestion, "??"},
        {TokenType::LeftBrace, "{"},
        {TokenType::StringStart, "\""},
        {TokenType::StringContent, "world"},
        {TokenType::StringEnd, "\""},
        {TokenType::Semicolon, ";"},
        {TokenType::RightBrace, "}"},
        {TokenType::Plus, "+"},
        {TokenType::Integer, "1"},
        {TokenType::StringBlockEnd, "}"},
        {TokenType::StringContent, "}}!"},
        {TokenType::StringEnd, "\""},
    });
    lex.require_eof();
}

TEST_CASE("New lexer should emit field accesses for integers following a dot operator", "[lexer]") {
    TestLexer lex(R"(
        a.0.1.2 . /* comment */ 3.foo
    )");

    lex.require_sequence({
        {TokenType::Identifier, "a"},
        {TokenType::Dot, "."},
        {TokenType::TupleField, "0"},
        {TokenType::Dot, "."},
        {TokenType::TupleField, "1"},
        {TokenType::Dot, "."},
        {TokenType::TupleField, "2"},
        {TokenType::Dot, "."},
        {TokenType::TupleField, "3"},
        {TokenType::Dot, "."},
        {TokenType::Identifier, "foo"},
    });
    lex.require_eof();
}
