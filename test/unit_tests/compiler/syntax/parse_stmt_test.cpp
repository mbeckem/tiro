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
                call_expr(var_expr("cleanup"), //
                    {
                        var_expr("foo"),
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
                arg_list({
                    var_expr("foo"),
                    simple_string("message"),
                }),
                token_type(TokenType::Semicolon),
            }));
}

TEST_CASE("Parser handles simple variable declarations", "[syntax]") {
    auto tree = parse_stmt_syntax("var f;");
    assert_parse_tree(tree,       //
        node(SyntaxType::VarStmt, //
            {
                node(SyntaxType::Var, //
                    {
                        token_type(TokenType::KwVar),
                        simple_binding(binding_name("f")),
                    }),
                token_type(TokenType::Semicolon),
            }));
}

TEST_CASE("Parser handles simple constant declarations with initializer", "[syntax]") {
    auto tree = parse_stmt_syntax("const f = 3;");
    assert_parse_tree(tree,       //
        node(SyntaxType::VarStmt, //
            {
                node(SyntaxType::Var, //
                    {
                        token_type(TokenType::KwConst),
                        simple_binding(binding_name("f"), literal(TokenType::Integer, "3")),
                    }),
                token_type(TokenType::Semicolon),
            }));
}

TEST_CASE("Parser handles tuple patterns in variable declarations", "[syntax]") {
    auto tree = parse_stmt_syntax("const (a, b) = f();");
    assert_parse_tree(tree,       //
        node(SyntaxType::VarStmt, //
            {
                node(SyntaxType::Var, //
                    {
                        token_type(TokenType::KwConst),
                        simple_binding(binding_tuple({"a", "b"}), node_type(SyntaxType::CallExpr)),
                    }),
                token_type(TokenType::Semicolon),
            }));
}

TEST_CASE("Parser handles variable declarations with multiple bindings", "[syntax]") {
    auto tree = parse_stmt_syntax("var a = 3, b, (c, d) = g();");
    assert_parse_tree(tree,       //
        node(SyntaxType::VarStmt, //
            {
                node(SyntaxType::Var, //
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

TEST_CASE("Parser handles for each loops", "[syntax]") {
    auto tree = parse_stmt_syntax(R"(
        for (a, b) in foo() {
            assert(a == b);
        }
    )");
    assert_parse_tree(tree,           //
        node(SyntaxType::ForEachStmt, //
            {
                token_type(TokenType::KwFor),
                binding_tuple({"a", "b"}),
                token_type(TokenType::KwIn),
                call_expr(var_expr("foo"), {}),
                node_type(SyntaxType::BlockExpr),
            }));
}

TEST_CASE("Parser handles classic for loops", "[syntax]") {
    auto tree = parse_stmt_syntax(R"(
        for var i = 0; i < 10; i += 1 {
            print(i);
        }
    )");
    assert_parse_tree(tree,       //
        node(SyntaxType::ForStmt, //
            {
                token_type(TokenType::KwFor),
                node(SyntaxType::ForStmtHeader, //
                    {
                        node_type(SyntaxType::Var),
                        token_type(TokenType::Semicolon),
                        node_type(SyntaxType::BinaryExpr),
                        token_type(TokenType::Semicolon),
                        node_type(SyntaxType::BinaryExpr),
                    }),
                node_type(SyntaxType::BlockExpr),
            }));
}

TEST_CASE("Parser handles classic for loops without variable declarations", "[syntax]") {
    auto tree = parse_stmt_syntax(R"(
        for ; i < 10; i += 1 {
            print(i);
        }
    )");
    assert_parse_tree(tree,       //
        node(SyntaxType::ForStmt, //
            {
                token_type(TokenType::KwFor),
                node(SyntaxType::ForStmtHeader, //
                    {
                        token_type(TokenType::Semicolon),
                        node_type(SyntaxType::BinaryExpr),
                        token_type(TokenType::Semicolon),
                        node_type(SyntaxType::BinaryExpr),
                    }),
                node_type(SyntaxType::BlockExpr),
            }));
}

TEST_CASE("Parser handles classic for loops without conditions", "[syntax]") {
    auto tree = parse_stmt_syntax(R"(
        for var i = 0; ; i += 1 {
            print(i);
        }
    )");
    assert_parse_tree(tree,       //
        node(SyntaxType::ForStmt, //
            {
                token_type(TokenType::KwFor),
                node(SyntaxType::ForStmtHeader, //
                    {
                        node_type(SyntaxType::Var),
                        token_type(TokenType::Semicolon),
                        token_type(TokenType::Semicolon),
                        node_type(SyntaxType::BinaryExpr),
                    }),
                node_type(SyntaxType::BlockExpr),
            }));
}

TEST_CASE("Parser handles classic for loops without update step", "[syntax]") {
    auto tree = parse_stmt_syntax(R"(
        for var i = 0; i < 10; {
            print(i);
        }
    )");
    assert_parse_tree(tree,       //
        node(SyntaxType::ForStmt, //
            {
                token_type(TokenType::KwFor),
                node(SyntaxType::ForStmtHeader, //
                    {
                        node_type(SyntaxType::Var),
                        token_type(TokenType::Semicolon),
                        node_type(SyntaxType::BinaryExpr),
                        token_type(TokenType::Semicolon),
                    }),
                node_type(SyntaxType::BlockExpr),
            }));
}

TEST_CASE("Parser handles while loops", "[syntax]") {
    auto tree = parse_stmt_syntax(R"(
        while 1 == 2 {
            print("hello world");
        }
    )");
    assert_parse_tree(tree,         //
        node(SyntaxType::WhileStmt, //
            {
                token_type(TokenType::KwWhile),
                node_type(SyntaxType::BinaryExpr),
                node_type(SyntaxType::BlockExpr),
            }));
}
