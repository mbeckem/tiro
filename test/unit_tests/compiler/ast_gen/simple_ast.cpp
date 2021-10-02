#include "./simple_ast.hpp"

#include "compiler/ast/ast.hpp"
#include "compiler/ast_gen/build_ast.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/syntax/build_syntax_tree.hpp"
#include "compiler/syntax/grammar/expr.hpp"
#include "compiler/syntax/grammar/item.hpp"
#include "compiler/syntax/grammar/stmt.hpp"
#include "compiler/syntax/lexer.hpp"
#include "compiler/syntax/parser.hpp"

#include <catch2/catch.hpp>

namespace tiro::test {

namespace {

std::vector<Token> tokenize(std::string_view source) {
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

SyntaxTree get_syntax_tree(std::string_view source, FunctionRef<void(Parser&)> parsefn) {
    auto tokens = tokenize(source);
    Parser parser(source, tokens);
    parsefn(parser);
    if (!parser.at(TokenType::Eof))
        FAIL_CHECK("Parser did not reach the end of file.");

    auto events = parser.take_events();
    return build_syntax_tree(source, events);
}

template<typename T, typename Builder>
SimpleAst<T> build_ast(Builder&& builder) {
    Diagnostics diag;
    SimpleAst<T> ast;
    ast.root = builder(ast.strings, diag);
    for (const auto& message : diag.messages()) {
        UNSCOPED_INFO("[DIAG] " << to_string(message.level) << ": " << message.text);
    }
    CHECK(diag.message_count() == 0);
    return ast;
}

} // namespace

SimpleAst<AstExpr> parse_expr_ast(std::string_view source) {
    auto syntax = get_syntax_tree(source, [&](Parser& p) { parse_expr(p, {}); });
    return build_ast<AstExpr>([&](StringTable& strings, Diagnostics& diag) {
        return build_expr_ast(syntax, strings, diag);
    });
}

SimpleAst<AstStmt> parse_stmt_ast(std::string_view source) {
    auto syntax = get_syntax_tree(source, [&](Parser& p) { parse_stmt(p, {}); });
    return build_ast<AstStmt>([&](StringTable& strings, Diagnostics& diag) {
        return build_stmt_ast(syntax, strings, diag);
    });
}

SimpleAst<AstStmt> parse_item_ast(std::string_view source) {
    auto syntax = get_syntax_tree(source, [&](Parser& p) { parse_item(p, {}); });
    return build_ast<AstStmt>([&](StringTable& strings, Diagnostics& diag) {
        return build_item_ast(syntax, strings, diag);
    });
}

SimpleAst<AstFile> parse_file_ast(std::string_view source) {
    auto syntax = get_syntax_tree(source, [&](Parser& p) { parse_file(p); });
    return build_ast<AstFile>([&](StringTable& strings, Diagnostics& diag) {
        return build_file_ast(syntax, strings, diag);
    });
}

SimpleAst<AstModule> parse_module_ast(const std::vector<std::string_view>& sources) {
    std::vector<SyntaxTreeEntry> files;

    u32 index = 0;
    for (const auto& source : sources) {
        SourceId id(index++);
        SyntaxTree tree = get_syntax_tree(source, [&](Parser& p) { parse_file(p); });
        files.emplace_back(id, std::move(tree));
    }
    return build_ast<AstModule>([&](StringTable& strings, Diagnostics& diag) {
        return build_module_ast(files, strings, diag);
    });
}

} // namespace tiro::test
