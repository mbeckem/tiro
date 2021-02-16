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

TEST_CASE("Parser handles simple variable declarations", "[syntax]") {
    auto tree = parse_stmt_syntax("var f;");
    assert_parse_tree(tree,           //
        node(SyntaxType::VarDeclStmt, //
            {
                node(SyntaxType::VarDecl, //
                    {
                        token_type(TokenType::KwVar),
                        simple_binding(binding_name("f")),
                    }),
                token_type(TokenType::Semicolon),
            }));
}

TEST_CASE("Parser handles simple constant declarations with initializer", "[syntax]") {
    auto tree = parse_stmt_syntax("const f = 3;");
    assert_parse_tree(tree,           //
        node(SyntaxType::VarDeclStmt, //
            {
                node(SyntaxType::VarDecl, //
                    {
                        token_type(TokenType::KwConst),
                        simple_binding(binding_name("f"), literal(TokenType::Integer, "3")),
                    }),
                token_type(TokenType::Semicolon),
            }));
}

TEST_CASE("Parser handles tuple patterns in variable declarations", "[syntax]") {
    auto tree = parse_stmt_syntax("const (a, b) = f();");
    assert_parse_tree(tree,           //
        node(SyntaxType::VarDeclStmt, //
            {
                node(SyntaxType::VarDecl, //
                    {
                        token_type(TokenType::KwConst),
                        simple_binding(binding_tuple({"a", "b"}), node_type(SyntaxType::CallExpr)),
                    }),
                token_type(TokenType::Semicolon),
            }));
}

TEST_CASE("Parser handles variable declarations with multiple bindings", "[syntax]") {
    auto tree = parse_stmt_syntax("var a = 3, b, (c, d) = g();");
    assert_parse_tree(tree,           //
        node(SyntaxType::VarDeclStmt, //
            {
                node(SyntaxType::VarDecl, //
                    {
                        token_type(TokenType::KwVar),
                        simple_binding(binding_name("a"), node_type(SyntaxType::Literal)),
                        token_type(TokenType::Comma),
                        simple_binding(binding_name("b")),
                        token_type(TokenType::Comma),
                        simple_binding(binding_tuple({"c", "d"}), node_type(SyntaxType::CallExpr)),
                    }),
                token_type(TokenType::Semicolon),
            }));
}
