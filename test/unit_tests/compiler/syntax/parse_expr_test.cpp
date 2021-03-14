#include <catch2/catch.hpp>

#include "./syntax_assert.hpp"

namespace tiro::test {

TEST_CASE("Parser should parse plain literals", "[syntax]") {
    struct Test {
        std::string_view source;
        TokenType expected_type;
    };

    Test tests[] = {
        {"true", TokenType::KwTrue},
        {"false", TokenType::KwFalse},
        {"null", TokenType::KwNull},
        {"#abc", TokenType::Symbol},
        {"1234", TokenType::Integer},
        {"123.456", TokenType::Float},
    };

    for (const auto& spec : tests) {
        auto tree = parse_expr_syntax(spec.source);
        assert_parse_tree(tree, literal(spec.expected_type, std::string(spec.source)));
    }
}

TEST_CASE("Parser should respect arithmetic operator precedence", "[syntax]") {
    std::string_view source = "-4**2 + 1234 * 2.34 - 1";

    auto tree = parse_expr_syntax(source);
    assert_parse_tree(tree,                      //
        binary_expr(TokenType::Minus,            //
            binary_expr(TokenType::Plus,         //
                binary_expr(TokenType::StarStar, //
                    unary_expr(TokenType::Minus, literal(TokenType::Integer, "4")),
                    literal(TokenType::Integer, "2")),
                binary_expr(TokenType::Star, //
                    literal(TokenType::Integer, "1234"), literal(TokenType::Float, "2.34"))),
            literal(TokenType::Integer, "1")));
}

TEST_CASE("Parser should respect operator precedence in assignments", "[syntax]") {
    std::string_view source = "a = b = 3 && 4";

    auto tree = parse_expr_syntax(source);
    assert_parse_tree(tree,            //
        binary_expr(TokenType::Equals, // a =
            var_expr("a"),
            binary_expr(TokenType::Equals, // b =
                var_expr("b"),
                binary_expr(TokenType::LogicalAnd, // 3 && 4
                    literal(TokenType::Integer, "3"), literal(TokenType::Integer, "4")))));
}

TEST_CASE("Parser should support binary assignment operators", "[syntax]") {
    auto tree = parse_expr_syntax("3 + (c = b -= 4 ** 2)");
    assert_parse_tree(tree,          //
        binary_expr(TokenType::Plus, //
            literal(TokenType::Integer, "3"),
            node(SyntaxType::GroupedExpr, //
                {
                    token_type(TokenType::LeftParen),
                    binary_expr(TokenType::Equals, //
                        var_expr("c"),
                        binary_expr(TokenType::MinusEquals, //
                            var_expr("b"),
                            binary_expr(TokenType::StarStar, //
                                literal(TokenType::Integer, "4"),
                                literal(TokenType::Integer, "2")))),
                    token_type(TokenType::RightParen),
                })));
}

TEST_CASE("Parser should support the null coalescing operator", "[syntax]") {
    auto tree = parse_expr_syntax("x.y ?? 3");
    assert_parse_tree(tree,                      //
        binary_expr(TokenType::QuestionQuestion, //
            member_expr(var_expr("x"), member("y")), literal(TokenType::Integer)));
}

TEST_CASE("Parser should respect the low precedence of the null coalescing operator", "[syntax]") {
    auto tree = parse_expr_syntax("x ?? 3 - 4");
    assert_parse_tree(tree,                      //
        binary_expr(TokenType::QuestionQuestion, //
            var_expr("x"),
            binary_expr(TokenType::Minus, //
                literal(TokenType::Integer, "3"), literal(TokenType::Integer, "4"))));
}

TEST_CASE("Parser handles grouped expressions", "[syntax]") {
    std::string_view source = "(a + b * 2)";

    auto tree = parse_expr_syntax(source);
    assert_parse_tree(tree,           //
        node(SyntaxType::GroupedExpr, //
            {
                token_type(TokenType::LeftParen),
                binary_expr(TokenType::Plus, //
                    var_expr("a"),
                    binary_expr(TokenType::Star, //
                        var_expr("b"), literal(TokenType::Integer, "2"))),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles empty tuple literals", "[syntax]") {
    auto tree = parse_expr_syntax("()");
    assert_parse_tree(tree,         //
        node(SyntaxType::TupleExpr, //
            {
                token_type(TokenType::LeftParen),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles single-element tuple literals", "[syntax]") {
    auto tree = parse_expr_syntax("(1,)");
    assert_parse_tree(tree,         //
        node(SyntaxType::TupleExpr, //
            {
                token_type(TokenType::LeftParen),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles tuple literals", "[syntax]") {
    auto tree = parse_expr_syntax("(1, 2, 3)");
    assert_parse_tree(tree,         //
        node(SyntaxType::TupleExpr, //
            {
                token_type(TokenType::LeftParen),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),
                literal(TokenType::Integer, "2"),
                token_type(TokenType::Comma),
                literal(TokenType::Integer, "3"),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles tuple literals with trailing commas", "[syntax]") {
    auto tree = parse_expr_syntax("(1, 2, 3,)");
    assert_parse_tree(tree,         //
        node(SyntaxType::TupleExpr, //
            {
                token_type(TokenType::LeftParen),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),
                literal(TokenType::Integer, "2"),
                token_type(TokenType::Comma),
                literal(TokenType::Integer, "3"),
                token_type(TokenType::Comma),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles empty record literals", "[syntax]") {
    auto tree = parse_expr_syntax("(:)");
    assert_parse_tree(tree,          //
        node(SyntaxType::RecordExpr, //
            {
                token_type(TokenType::LeftParen),
                token_type(TokenType::Colon),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles record literals", "[syntax]") {
    auto tree = parse_expr_syntax("(a: b, c: 1)");
    assert_parse_tree(tree,          //
        node(SyntaxType::RecordExpr, //
            {
                token_type(TokenType::LeftParen),
                name("a"),
                token_type(TokenType::Colon),
                var_expr("b"),
                token_type(TokenType::Comma),
                name("c"),
                token_type(TokenType::Colon),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles record literals with trailing comma", "[syntax]") {
    auto tree = parse_expr_syntax("(a: b, c: 1,)");
    assert_parse_tree(tree,          //
        node(SyntaxType::RecordExpr, //
            {
                token_type(TokenType::LeftParen),
                name("a"),
                token_type(TokenType::Colon),
                var_expr("b"),
                token_type(TokenType::Comma),
                name("c"),
                token_type(TokenType::Colon),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles empty array literals", "[syntax]") {
    auto tree = parse_expr_syntax("[]");
    assert_parse_tree(tree,         //
        node(SyntaxType::ArrayExpr, //
            {
                token_type(TokenType::LeftBracket),
                token_type(TokenType::RightBracket),
            }));
}

TEST_CASE("Parser handles array literals", "[syntax]") {
    auto tree = parse_expr_syntax("[1, 2]");
    assert_parse_tree(tree,         //
        node(SyntaxType::ArrayExpr, //
            {
                token_type(TokenType::LeftBracket),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),
                literal(TokenType::Integer, "2"),
                token_type(TokenType::RightBracket),
            }));
}

TEST_CASE("Parser handles array literals with trailing comma", "[syntax]") {
    auto tree = parse_expr_syntax("[1, 2,]");
    assert_parse_tree(tree,         //
        node(SyntaxType::ArrayExpr, //
            {
                token_type(TokenType::LeftBracket),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),
                literal(TokenType::Integer, "2"),
                token_type(TokenType::Comma),
                token_type(TokenType::RightBracket),
            }));
}

TEST_CASE("Parser handles member access", "[syntax]") {
    auto tree = parse_expr_syntax("a?.b.c");
    assert_parse_tree(tree, //
        member_expr(member_expr(var_expr("a"), member("b"), true), member("c")));
}

TEST_CASE("Parser handles tuple members", "[syntax]") {
    auto tree = parse_expr_syntax("a.0.1");
    assert_parse_tree(tree, //
        member_expr(member_expr(var_expr("a"), member(0)), member(1)));
}

TEST_CASE("Parser handles array access", "[syntax]") {
    auto tree = parse_expr_syntax("a[b?[c]]");
    assert_parse_tree(tree,    //
        index_expr(            //
            var_expr("a"),     //
            index_expr(        //
                var_expr("b"), //
                var_expr("c"), true)));
}

TEST_CASE("Parser handles function calls", "[syntax]") {
    auto tree = parse_expr_syntax("f(1)(2, 3)()");
    assert_parse_tree(tree,                              //
        call_expr(                                       //
            call_expr(                                   //
                call_expr(var_expr("f"),                 //
                    {literal(TokenType::Integer, "1")}), //
                {
                    literal(TokenType::Integer, "2"),
                    literal(TokenType::Integer, "3"),
                }),
            {}));
}

TEST_CASE("Parser handles optional function calls", "[syntax]") {
    auto tree = parse_expr_syntax("f(1)?(2, 3)");
    assert_parse_tree(tree,          //
        call_expr(                   //
            call_expr(var_expr("f"), //
                {
                    literal(TokenType::Integer, "1"),
                }),
            {
                literal(TokenType::Integer, "2"),
                literal(TokenType::Integer, "3"),
            },
            true));
}

TEST_CASE("Parser handles simple strings", "[syntax]") {
    auto tree = parse_expr_syntax("\"hello world\"");
    assert_parse_tree(tree, simple_string("hello world"));
}

TEST_CASE("Parser handles strings with variable interpolation", "[syntax]") {
    auto tree = parse_expr_syntax("\"hello $name!\"");
    assert_parse_tree(tree, //
        full_string({
            string_content("hello "),
            string_var("name"),
            string_content("!"),
        }));
}

TEST_CASE("Parser handles strings with interpolated expressions", "[syntax]") {
    auto tree = parse_expr_syntax("\"hello ${a.b.get_name()}!\"");
    assert_parse_tree(tree, //
        full_string({
            string_content("hello "),
            string_block(call_expr(
                member_expr(member_expr(var_expr("a"), member("b")), member("get_name")), {})),
            string_content("!"),
        }));
}

TEST_CASE("Parser merges sequences of strings into string groups", "[syntax]") {
    auto tree = parse_expr_syntax("\"foo\"'bar'\"baz\"");
    assert_parse_tree(tree,           //
        node(SyntaxType::StringGroup, //
            {
                simple_string("foo"),
                simple_string("bar"),
                simple_string("baz"),
            }));
}

TEST_CASE("Parser handles block expressions", "[syntax]") {
    auto tree = parse_expr_syntax("{ a; 4; }");
    assert_parse_tree(tree,         //
        node(SyntaxType::BlockExpr, //
            {
                token_type(TokenType::LeftBrace),
                node(SyntaxType::ExprStmt, //
                    {
                        var_expr("a"),
                        token_type(TokenType::Semicolon),
                    }),
                node(SyntaxType::ExprStmt, //
                    {
                        literal(TokenType::Integer, "4"),
                        token_type(TokenType::Semicolon),
                    }),
                token_type(TokenType::RightBrace),
            }));
}

TEST_CASE("Parser handles empty block expressions", "[syntax]") {
    auto tree = parse_expr_syntax("{}");
    assert_parse_tree(tree,         //
        node(SyntaxType::BlockExpr, //
            {
                token_type(TokenType::LeftBrace),
                token_type(TokenType::RightBrace),
            }));
}

TEST_CASE("Parser handles block expressions with redundant semicolons", "[syntax]") {
    auto tree = parse_expr_syntax("{;;1;;}");
    assert_parse_tree(tree,         //
        node(SyntaxType::BlockExpr, //
            {
                token_type(TokenType::LeftBrace),
                token_type(TokenType::Semicolon),
                token_type(TokenType::Semicolon),
                node(SyntaxType::ExprStmt, //
                    {
                        literal(TokenType::Integer, "1"),
                        token_type(TokenType::Semicolon),
                    }),
                token_type(TokenType::Semicolon),
                token_type(TokenType::RightBrace),
            }));
}

TEST_CASE("Parser handles if expressions", "[syntax]") {
    auto tree = parse_expr_syntax("if a { return 3; } else if (1) { } else { 1; }");
    assert_parse_tree(tree,      //
        node(SyntaxType::IfExpr, //
            {
                // If
                token_type(TokenType::KwIf),
                node(SyntaxType::Condition,
                    {
                        var_expr("a"),
                    }),

                // Then
                node_type(SyntaxType::BlockExpr),

                // Else If
                token_type(TokenType::KwElse),
                node(SyntaxType::IfExpr, //
                    {
                        // If
                        token_type(TokenType::KwIf),
                        node(SyntaxType::Condition,
                            {
                                node_type(SyntaxType::GroupedExpr),
                            }),

                        // Then
                        node_type(SyntaxType::BlockExpr),

                        // Else
                        token_type(TokenType::KwElse),
                        node_type(SyntaxType::BlockExpr),
                    }),
            }));
}

TEST_CASE("Parser handles function expressions", "[syntax]") {
    auto tree = parse_expr_syntax("func my_func (a, b) { return a + b; }");
    assert_parse_tree(tree,         //
        node(SyntaxType::FuncExpr,  //
            {node(SyntaxType::Func, //
                {
                    token_type(TokenType::KwFunc),
                    name("my_func"),
                    param_list({
                        token(TokenType::Identifier, "a"),
                        token(TokenType::Identifier, "b"),
                    }),
                    node_type(SyntaxType::BlockExpr),
                })}));
}

TEST_CASE("Parser handles function expressions with value body", "[syntax]") {
    auto tree = parse_expr_syntax("func my_func (a, b) = a * b");
    assert_parse_tree(tree,        //
        node(SyntaxType::FuncExpr, //
            {
                node(SyntaxType::Func, //
                    {
                        token_type(TokenType::KwFunc),
                        name("my_func"),
                        param_list({
                            token(TokenType::Identifier, "a"),
                            token(TokenType::Identifier, "b"),
                        }),
                        token_type(TokenType::Equals),
                        node(SyntaxType::BinaryExpr,
                            {
                                var_expr("a"),
                                token_type(TokenType::Star),
                                var_expr("b"),
                            }),
                    }),
            }));
}

TEST_CASE("Parser handles set literals", "[syntax]") {
    auto tree = parse_expr_syntax("set { a, 1, f() }");
    assert_parse_tree(tree,             //
        node(SyntaxType::ConstructExpr, //
            {
                token(TokenType::Identifier, "set"),
                token_type(TokenType::LeftBrace),

                node_type(SyntaxType::VarExpr),
                token_type(TokenType::Comma),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),
                node_type(SyntaxType::CallExpr),

                token_type(TokenType::RightBrace),
            }));
}

TEST_CASE("Parser handles map literals", "[syntax]") {
    auto tree = parse_expr_syntax("map { a : 1, g() : f() }");
    assert_parse_tree(tree,             //
        node(SyntaxType::ConstructExpr, //
            {
                token(TokenType::Identifier, "map"),
                token_type(TokenType::LeftBrace),

                node_type(SyntaxType::VarExpr),
                token_type(TokenType::Colon),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),

                call_expr(var_expr("g"), {}),
                token_type(TokenType::Colon),
                call_expr(var_expr("f"), {}),

                token_type(TokenType::RightBrace),
            }));
}

} // namespace tiro::test
