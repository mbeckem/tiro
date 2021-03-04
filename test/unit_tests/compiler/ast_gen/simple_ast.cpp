#include "./simple_ast.hpp"

#include "compiler/ast/ast.hpp"
#include "compiler/ast_gen/build_ast.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/syntax/build_syntax_tree.hpp"
#include "compiler/syntax/grammar/expr.hpp"
#include "compiler/syntax/lexer.hpp"
#include "compiler/syntax/parser.hpp"

#include <catch2/catch.hpp>

namespace tiro::next::test {

namespace {

struct TestHelper {
public:
    TestHelper(std::string_view source)
        : source_(source)
        , tokens_(tokenize(source))
        , parser_(tokens_) {}

    Parser& parser() { return parser_; }

    SyntaxTree get_syntax_tree();

private:
    static std::vector<Token> tokenize(std::string_view source);

private:
    std::string_view source_;
    std::vector<Token> tokens_;
    Parser parser_;
};

} // namespace

SimpleAst<AstExpr> parse_expr_ast(std::string_view source) {
    TestHelper h(source);
    parse_expr(h.parser(), {});

    Diagnostics diag;
    SimpleAst<AstExpr> ast;

    ast.root = build_expr_ast(h.get_syntax_tree(), ast.strings, diag);
    for (const auto& message : diag.messages()) {
        UNSCOPED_INFO("[DIAG] " << to_string(message.level) << ": " << message.text);
    }

    CHECK(diag.message_count() == 0);
    return ast;
}

SyntaxTree TestHelper::get_syntax_tree() {
    if (!parser_.at(TokenType::Eof))
        FAIL_CHECK("Parser did not reach the end of file.");

    auto events = parser_.take_events();
    return build_syntax_tree(source_, events);
}

std::vector<Token> TestHelper::tokenize(std::string_view source) {
    Lexer lexer(source);
    lexer.ignore_comments(true);

    std::vector<Token> tokens;
    while (1) {
        auto token = lexer.next();
        tokens.push_back(token);
        if (token.type() == TokenType::Eof)
            break;
    }
    return tokens;
}

} // namespace tiro::next::test
