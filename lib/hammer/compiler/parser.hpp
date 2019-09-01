#ifndef HAMMER_COMPILER_PARSER_HPP
#define HAMMER_COMPILER_PARSER_HPP

#include "hammer/ast/fwd.hpp"
#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/lexer.hpp"
#include "hammer/compiler/source_reference.hpp"
#include "hammer/compiler/string_table.hpp"
#include "hammer/core/casting.hpp"
#include "hammer/core/span.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace hammer {

class Parser {
public:
    explicit Parser(std::string_view file_name, std::string_view source,
        StringTable& strings, Diagnostics& diag);

    Diagnostics& diag() { return diag_; }

    // Parses a file. A file is a sequence of top level items (functions, classes etc.)
    std::unique_ptr<ast::File> parse_file();

    // Parses a toplevel item (e.g. an import or a function declaration).
    std::unique_ptr<ast::Node> parse_toplevel_item(TokenTypes follow);

private:
    // Parses an import declaration.
    std::unique_ptr<ast::ImportDecl> parse_import_decl();

    // Parses a function declaration.
    std::unique_ptr<ast::FuncDecl> parse_func_decl(
        bool requires_name, TokenTypes follow);

public:
    // Parses a single statement.
    std::unique_ptr<ast::Stmt> parse_stmt(TokenTypes follow);

private:
    // Parses a variable / constant declaration.
    std::unique_ptr<ast::DeclStmt> parse_var_decl(TokenTypes follow);

    // Parses a while loop statement.
    std::unique_ptr<ast::WhileStmt> parse_while_stmt(TokenTypes follow);

    // Parses a for loop statement.
    std::unique_ptr<ast::ForStmt> parse_for_stmt(TokenTypes follow);

    // Parses an expression and wraps it into an expression statement.
    std::unique_ptr<ast::ExprStmt> parse_expr_stmt(TokenTypes follow);

public:
    // Parses an expression. Public for testing.
    std::unique_ptr<ast::Expr> parse_expr(TokenTypes follow);

private:
    // Recursive parsing function for expressions with infix operators.
    std::unique_ptr<ast::Expr> parse_expr_precedence(
        int min_precedence, TokenTypes follow);

    // Parses an expression preceeded by unary operators.
    std::unique_ptr<ast::Expr> parse_prefix_expr(TokenTypes follow);

    // Parses suffix expression, i.e. an expression followed by one (or more) function calls, dotted names, function/method calls, index expressions etc.
    std::unique_ptr<ast::Expr> parse_suffix_expr(TokenTypes follow);
    std::unique_ptr<ast::Expr> parse_suffix_expr_inner(
        std::unique_ptr<ast::Expr> expr);

    // Parses primary expressions (constants, variables, function calls, braced expressions ...)
    std::unique_ptr<ast::Expr> parse_primary_expr(TokenTypes follow);

    // Parses a block expression.
    std::unique_ptr<ast::BlockExpr> parse_block_expr(TokenTypes follow);

    // Parses an if expression.
    std::unique_ptr<ast::IfExpr> parse_if_expr(TokenTypes follow);

    // Returns true if we're at the start of a variable declaration.
    static bool can_begin_var_decl(TokenType type);

    // Returns true if the token type would be a valid start for an expression,
    // e.g. identifiers, literals.
    static bool can_begin_expression(TokenType type);

    // Creates a source reference instance for the given range [begin, end).
    SourceReference ref(size_t begin, size_t end) const;

    template<typename Operation>
    auto check_error(std::string_view context, TokenTypes first,
        TokenTypes follow, Operation&& op);

    // Returns the current token if its type is a member of the provided set.
    // Advances the input in that case.
    // Does nothing otherwise.
    std::optional<Token> accept(TokenTypes tokens);

    // Like "accept", but emits an error if the token is of any different type.
    std::optional<Token> expect(TokenTypes tokens);

    // Moves to the next token.
    void advance();

private:
    InternedString file_name_;
    std::string_view source_;
    StringTable& strings_;
    Diagnostics& diag_;

    Lexer lexer_;
    Token current_;
};

} // namespace hammer

#endif // HAMMER_COMPILER_PARSER_HPP
