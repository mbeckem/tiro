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
