#include "./simple_ast.hpp"

#include "compiler/ast/ast.hpp"
#include "compiler/ast_gen/build_ast.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/syntax/build_syntax_tree.hpp"
#include "compiler/syntax/grammar/expr.hpp"
#include "compiler/syntax/grammar/stmt.hpp"
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

    template<typename T, typename ParseFunction, typename BuildFunction>
    SimpleAst<T> build_ast(ParseFunction&& pf, BuildFunction&& bf) {
        Diagnostics diag;
        pf(parser_);

        SimpleAst<T> ast;
        ast.root = bf(get_syntax_tree(), ast.strings, diag);
        for (const auto& message : diag.messages()) {
            UNSCOPED_INFO("[DIAG] " << to_string(message.level) << ": " << message.text);
        }

        CHECK(diag.message_count() == 0);
        return ast;
    }

private:
    static std::vector<Token> tokenize(std::string_view source);

    SyntaxTree get_syntax_tree();

private:
    std::string_view source_;
    std::vector<Token> tokens_;
    Parser parser_;
};

} // namespace

static void parse_expr_impl(Parser& p) {
    parse_expr(p, {});
}

static void parse_stmt_impl(Parser& p) {
    parse_stmt(p, {});
}

SimpleAst<AstExpr> parse_expr_ast(std::string_view source) {
    TestHelper h(source);
    return h.build_ast<AstExpr>(parse_expr_impl, build_expr_ast);
}

SimpleAst<AstStmt> parse_stmt_ast(std::string_view source) {
    TestHelper h(source);
    return h.build_ast<AstStmt>(parse_stmt_impl, build_stmt_ast);
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
