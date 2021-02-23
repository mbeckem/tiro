#include <catch2/catch.hpp>

#include "./syntax_assert.hpp"

using namespace tiro;
using namespace tiro::next;
using namespace tiro::next::test;

TEST_CASE("Parser handles import items", "[syntax])") {
    auto tree = parse_item_syntax("import foo.bar.baz;");
    assert_parse_tree(tree,          //
        node(SyntaxType::ImportItem, //
            {
                token_type(TokenType::KwImport),
                token(TokenType::Identifier, "foo"),
                token_type(TokenType::Dot),
                token(TokenType::Identifier, "bar"),
                token_type(TokenType::Dot),
                token(TokenType::Identifier, "baz"),
                token_type(TokenType::Semicolon),
            }));
}

TEST_CASE("Parser handles function declarations with modifiers", "[syntax]") {
    auto tree = parse_item_syntax("export func foo(a, b) { return 123; }");
    assert_parse_tree(tree,        //
        node(SyntaxType::FuncItem, //
            {
                node(SyntaxType::Func, //
                    {
                        node(SyntaxType::Modifiers,
                            {
                                token_type(TokenType::KwExport),
                            }),
                        token_type(TokenType::KwFunc),
                        name("foo"),
                        param_list({
                            token(TokenType::Identifier, "a"),
                            token(TokenType::Identifier, "b"),
                        }),
                        node_type(SyntaxType::BlockExpr),
                    }),
            }));
}

TEST_CASE("Parser handles short function declarations at top level", "[syntax]") {
    auto tree = parse_item_syntax("export func foo(a, b) = a + b;");
    assert_parse_tree(tree,        //
        node(SyntaxType::FuncItem, //
            {
                node(SyntaxType::Func, //
                    {
                        node_type(SyntaxType::Modifiers),
                        token_type(TokenType::KwFunc),
                        name("foo"),
                        param_list({
                            token(TokenType::Identifier, "a"),
                            token(TokenType::Identifier, "b"),
                        }),
                        token_type(TokenType::Equals),
                        node_type(SyntaxType::BinaryExpr),
                    }),
                token_type(TokenType::Semicolon),
            }));
}

TEST_CASE("Parser handles variable declaration at top level", "[syntax]") {
    auto tree = parse_item_syntax("export const (a, b) = init();");
    assert_parse_tree(tree,       //
        node(SyntaxType::VarItem, //
            {
                node(SyntaxType::Var,
                    {
                        node(SyntaxType::Modifiers,
                            {
                                token_type(TokenType::KwExport),
                            }),
                        token_type(TokenType::KwConst),
                        simple_binding(binding_tuple({"a", "b"}), call_expr(var_expr("init"), {})),
                    }),
                token_type(TokenType::Semicolon),
            }));
}

TEST_CASE("Parser handles files", "[syntax]") {
    auto tree = parse_file_syntax(R"(
        import foo.bar;

        var foo = 123;

        const (a, b) = f();

        ;

        export func fn() {
            return a + b;
        }
    )");

    assert_parse_tree(tree,    //
        node(SyntaxType::File, //
            {
                node_type(SyntaxType::ImportItem),
                node_type(SyntaxType::VarItem),
                node_type(SyntaxType::VarItem),
                token_type(TokenType::Semicolon),
                node_type(SyntaxType::FuncItem),
            }));
}
