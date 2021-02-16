#include <catch2/catch.hpp>

#include "./syntax_assert.hpp"

using namespace tiro;
using namespace tiro::next;

TEST_CASE("Parser handles defer statements", "[syntax]") {
    auto tree = parse_stmt_syntax("defer cleanup(foo);");
    assert_parse_tree(tree,         //
        node(SyntaxType::DeferStmt, //
            {
                token_type(TokenType::KwDefer),
                call_expr(name("cleanup"), //
                    {
                        name("foo"),
                    }),
                token_type(TokenType::Semicolon),
            }));
}

TEST_CASE("Parser handles assert statements", "[syntax]") {
    auto tree = parse_stmt_syntax("assert(foo, \"message\");");
    assert_parse_tree(tree,          //
        node(SyntaxType::AssertStmt, //
            {
                token_type(TokenType::KwAssert),
                node(SyntaxType::ArgList, //
                    {
                        token_type(TokenType::LeftParen),
                        name("foo"),
                        token_type(TokenType::Comma),
                        simple_string("message"),
                        token_type(TokenType::RightParen),
                    }),
                token_type(TokenType::Semicolon),
            }));
}
