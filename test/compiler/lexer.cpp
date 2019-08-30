#include "hammer/compiler/lexer.hpp"

#include "hammer/compiler/diagnostics.hpp"

#include <catch.hpp>

using namespace hammer;

template<typename Test>
void with_content(std::string_view file_content, Test&& test) {
    StringTable strings;
    Diagnostics diag;
    InternedString file_name = strings.insert("unit-test");

    Lexer lex(file_name, file_content, strings, diag);
    test(lex);
}

TEST_CASE("lex numeric literals", "[lexer]") {
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

        with_content(test.source, [&](Lexer& l) {
            Token tok = l.next();

            REQUIRE(l.diag().message_count() == 0);

            REQUIRE(tok.source().begin() == 0);
            REQUIRE(tok.source().end() == test.source.size());
            REQUIRE_FALSE(tok.has_error());

            if (std::holds_alternative<i64>(test.expected)) {
                REQUIRE(tok.type() == TokenType::integer_literal);

                i64 value = tok.int_value();
                REQUIRE(value == std::get<i64>(test.expected));
            } else if (std::holds_alternative<double>(test.expected)) {
                REQUIRE(tok.type() == TokenType::float_literal);

                double value = tok.float_value();
                REQUIRE(value == std::get<double>(test.expected));
            } else {
                FAIL();
            }
        });
    }
}

TEST_CASE("lex error when alphabetic characters are read after a number", "[lexer]") {
    std::string_view source = "123aaaa";

    with_content(source, [&](Lexer& l) {
        Token tok = l.next();
        REQUIRE(tok.type() == TokenType::integer_literal);

        Diagnostics& diag = l.diag();
        REQUIRE(diag.message_count() > 0);
        REQUIRE(diag.has_errors());
    });
}

TEST_CASE("lex string literals", "[lexer]") {
    struct test_t {
        std::string_view source;
        std::string expected;
    } tests[] = {
        {"\"hello world\"", "hello world"},
        {"'hello world'", "hello world"},
        {"'escape \\r\\n'", "escape \r\n"},
        {"\"\\\"\"", "\""},
    };

    for (const test_t& test : tests) {
        CAPTURE(test.source);

        with_content(test.source, [&](Lexer& l) {
            Token tok = l.next();

            REQUIRE(l.diag().message_count() == 0);

            REQUIRE(tok.source().begin() == 0);
            REQUIRE(tok.source().end() == test.source.size());
            REQUIRE_FALSE(tok.has_error());

            REQUIRE(tok.type() == TokenType::string_literal);
            REQUIRE(l.strings().value(tok.string_value()) == test.expected);
        });
    }
}

TEST_CASE("lex identifiers", "[lexer]") {
    std::string_view source = "a aa a123 a_b_c _1";

    struct expected_t {
        size_t start;
        size_t end;
        std::string name;
    } expected_identifiers[] = {{0, 1, "a"}, {2, 4, "aa"}, {5, 9, "a123"}, {10, 15, "a_b_c"}, {16,18, "_1"}};

    with_content(source, [&](Lexer& l) {
        for (const expected_t& expected : expected_identifiers) {
            CAPTURE(expected.name);

            Token tok = l.next();
            REQUIRE_FALSE(tok.has_error());
            REQUIRE(l.diag().message_count() == 0);
            REQUIRE(tok.type() == TokenType::identifier);
            REQUIRE(l.strings().value(tok.string_value()) == expected.name);
            REQUIRE(tok.source().begin() == expected.start);
            REQUIRE(tok.source().end() == expected.end);
        }

        Token last = l.next();
        CAPTURE(to_token_name(last.type()));
        REQUIRE_FALSE(last.has_error());
        REQUIRE(l.diag().message_count() == 0);
        REQUIRE(last.type() == TokenType::eof);
    });
}

TEST_CASE("lex operators", "[lexer]") {
    std::string_view source =
        "( ) [ ] { } . , : ; ? + - * ** / % "
        "++ -- ~ | ^ & ! || && = == != "
        "< > <= >=";

    TokenType expected_tokens[] = {
        TokenType::lparen,   TokenType::rparen,    TokenType::lbracket,   TokenType::rbracket,
        TokenType::lbrace,   TokenType::rbrace,    TokenType::dot,        TokenType::comma,
        TokenType::colon,    TokenType::semicolon, TokenType::question,   TokenType::plus,
        TokenType::minus,    TokenType::star,      TokenType::starstar,   TokenType::slash,
        TokenType::percent,  TokenType::plusplus,  TokenType::minusminus, TokenType::bnot,
        TokenType::bor,      TokenType::bxor,      TokenType::band,       TokenType::lnot,
        TokenType::lor,      TokenType::land,      TokenType::eq,         TokenType::eqeq,
        TokenType::neq,      TokenType::less,      TokenType::greater,    TokenType::lesseq,
        TokenType::greatereq};

    with_content(source, [&](Lexer& l) {
        for (TokenType expected : expected_tokens) {
            CAPTURE(to_token_name(expected));

            Token tok = l.next();
            CAPTURE(to_token_name(tok.type()));
            REQUIRE_FALSE(tok.has_error());
            REQUIRE(l.diag().message_count() == 0);
            REQUIRE(tok.type() == expected);
        }

        Token last = l.next();
        CAPTURE(to_token_name(last.type()));
        REQUIRE_FALSE(last.has_error());
        REQUIRE(l.diag().message_count() == 0);
        REQUIRE(last.type() == TokenType::eof);
    });
}

TEST_CASE("lex keywords", "[lexer]") {
    std::string_view source =
        "func var const if else while for "
        "continue break switch class struct "
        "protocol true false null import export package "
        "yield async await throw try catch scope";

    TokenType expected_tokens[] = {
        TokenType::kw_func,     TokenType::kw_var,    TokenType::kw_const,   TokenType::kw_if,
        TokenType::kw_else,     TokenType::kw_while,  TokenType::kw_for,     TokenType::kw_continue,
        TokenType::kw_break,    TokenType::kw_switch, TokenType::kw_class,   TokenType::kw_struct,
        TokenType::kw_protocol, TokenType::kw_true,   TokenType::kw_false,   TokenType::kw_null,
        TokenType::kw_import,   TokenType::kw_export, TokenType::kw_package, TokenType::kw_yield,
        TokenType::kw_async,    TokenType::kw_await,  TokenType::kw_throw,   TokenType::kw_try,
        TokenType::kw_catch,    TokenType::kw_scope,
    };

    with_content(source, [&](Lexer& l) {
        for (TokenType expected : expected_tokens) {
            CAPTURE(to_token_name(expected));

            Token tok = l.next();
            CAPTURE(to_token_name(tok.type()));
            REQUIRE_FALSE(tok.has_error());
            REQUIRE(l.diag().message_count() == 0);
            REQUIRE(tok.type() == expected);
        }

        Token last = l.next();
        CAPTURE(to_token_name(last.type()));
        REQUIRE_FALSE(last.has_error());
        REQUIRE(l.diag().message_count() == 0);
        REQUIRE(last.type() == TokenType::eof);
    });
}

TEST_CASE("lex block comments", "[lexer]") {
    std::string_view source = "hello/*world*/;";

    with_content(source, [&](Lexer& l) {
        l.ignore_comments(true);

        Token tok_ident = l.next();
        REQUIRE(tok_ident.type() == TokenType::identifier);
        REQUIRE_FALSE(tok_ident.has_error());
        REQUIRE(l.strings().value(tok_ident.string_value()) == "hello");

        Token tok_semi = l.next();
        REQUIRE(tok_semi.type() == TokenType::semicolon);
        REQUIRE(!tok_semi.has_error());

        REQUIRE(l.diag().message_count() == 0);
    });

    with_content(source, [&](Lexer& l) {
        l.ignore_comments(false);

        Token tok_ident = l.next();
        REQUIRE(tok_ident.type() == TokenType::identifier);
        REQUIRE_FALSE(tok_ident.has_error());
        REQUIRE(l.strings().value(tok_ident.string_value()) == "hello");

        Token tok_comment = l.next();
        REQUIRE(tok_comment.type() == TokenType::comment);
        REQUIRE_FALSE(tok_comment.has_error());

        size_t begin = tok_comment.source().begin();
        size_t end = tok_comment.source().end();
        REQUIRE(end >= begin);
        REQUIRE(source.substr(begin, end - begin) == "/*world*/");

        Token tok_semi = l.next();
        REQUIRE(tok_semi.type() == TokenType::semicolon);
        REQUIRE(!tok_semi.has_error());

        REQUIRE(l.diag().message_count() == 0);
    });
}

TEST_CASE("lex line comment", "[lexer]") {
    std::string_view source = "asd // + - test;\n [";

    with_content(source, [&](Lexer& l) {
        l.ignore_comments(false);

        Token tok_ident = l.next();
        REQUIRE(tok_ident.type() == TokenType::identifier);
        REQUIRE_FALSE(tok_ident.has_error());
        REQUIRE(l.strings().value(tok_ident.string_value()) == "asd");

        Token tok_comment = l.next();
        REQUIRE(tok_comment.type() == TokenType::comment);
        REQUIRE_FALSE(tok_comment.has_error());

        size_t begin = tok_comment.source().begin();
        size_t end = tok_comment.source().end();
        REQUIRE(end >= begin);
        REQUIRE(source.substr(begin, end - begin) == "// + - test;");

        Token tok_semi = l.next();
        REQUIRE(tok_semi.type() == TokenType::lbracket);
        REQUIRE(!tok_semi.has_error());

        REQUIRE(l.diag().message_count() == 0);
    });
}

TEST_CASE("lex nested block comment", "[lexer]") {
    std::string source = "   /* 1 /* 2 /* 3 */ 4 */ 5 */   ";

    with_content(source, [&](Lexer& l) {
        l.ignore_comments(false);

        Token tok_comment = l.next();
        REQUIRE(tok_comment.type() == TokenType::comment);
        REQUIRE_FALSE(tok_comment.has_error());

        size_t begin = tok_comment.source().begin();
        size_t end = tok_comment.source().end();
        REQUIRE(end >= begin);
        REQUIRE(source.substr(begin, end - begin) == "/* 1 /* 2 /* 3 */ 4 */ 5 */");

        Token tok_eof = l.next();
        REQUIRE(tok_eof.type() == TokenType::eof);

        REQUIRE(l.diag().message_count() == 0);
    });
}
