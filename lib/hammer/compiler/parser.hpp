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
    class ErrorTag {};

    template<typename Node>
    class [[nodiscard]] Result {
    public:
        static_assert(std::is_base_of_v<ast::Node, Node>);

        using value_type = std::unique_ptr<Node>;

        // Failure and no node value at all.
        Result(ErrorTag)
            : node_(nullptr)
            , parse_ok_(false) {}

        template<typename OtherNode,
            std::enable_if_t<std::is_base_of_v<Node, OtherNode>>* = nullptr>
        Result(std::unique_ptr<OtherNode> && node, bool parse_ok = true)
            : node_(std::move(node))
            , parse_ok_(node_ != nullptr && parse_ok) {

            HAMMER_ASSERT(!parse_ok_ || node_ != nullptr,
                "Node must be non-null if parsing succeeded.");
        }

        template<typename OtherNode,
            std::enable_if_t<!std::is_same_v<OtherNode,
                                 Node> && std::is_base_of_v<Node, OtherNode>>* =
                nullptr>
        Result(Result<OtherNode> && other)
            : node_(std::move(other.node_))
            , parse_ok_(other.parse_ok_) {
            HAMMER_ASSERT(!parse_ok_ || node_ != nullptr,
                "Node must be non-null if parsing succeeded.");
        }

        // True if no parse error occurred. False if the parser must synchronize.
        explicit operator bool() const { return parse_ok_; }

        bool parse_ok() const { return parse_ok_; }

        // True if we have a (partial or completely valid) node stored.
        bool has_node() const { return node_ != nullptr; }

        // May be completely parsed node, a partial node (with has_error() == true) or null.
        std::unique_ptr<Node> take_node() { return std::move(node_); }

        // Calls the function if this result holds a non-null node.
        template<typename F>
        void with_node(F && function) {
            if (node_)
                function(std::move(node_));
        }

    private:
        // The result of the parse operation (or null).
        value_type node_;

        // True if parsing failed and we have to seek to a synchronizing token
        bool parse_ok_ = false;

        template<typename OtherNode>
        friend class Result;
    };

public:
    explicit Parser(std::string_view file_name, std::string_view source,
        StringTable& strings, Diagnostics& diag);

    Diagnostics& diag() { return diag_; }

    // Parses a file. A file is a sequence of top level items (functions, classes etc.)
    Result<ast::File> parse_file();

    // Parses a toplevel item (e.g. an import or a function declaration).
    Result<ast::Node> parse_toplevel_item(TokenTypes sync);

private:
    // Parses an import declaration.
    Result<ast::ImportDecl> parse_import_decl();

    // Parses a function declaration.
    Result<ast::FuncDecl> parse_func_decl(bool requires_name, TokenTypes sync);

public:
    // Parses a single statement.
    Result<ast::Stmt> parse_stmt(TokenTypes sync);

private:
    // Parses a variable / constant declaration.
    Result<ast::DeclStmt> parse_var_decl(TokenTypes sync);

    // Parses a while loop statement.
    Result<ast::WhileStmt> parse_while_stmt(TokenTypes sync);

    // Parses a for loop statement.
    Result<ast::ForStmt> parse_for_stmt(TokenTypes sync);

    // Parses an expression and wraps it into an expression statement.
    Result<ast::ExprStmt> parse_expr_stmt(TokenTypes sync);

public:
    // Parses an expression. Public for testing.
    Result<ast::Expr> parse_expr(TokenTypes sync);

private:
    // Recursive parsing function for expressions with infix operators.
    Result<ast::Expr>
    parse_expr_precedence(int min_precedence, TokenTypes sync);

    // Parses an expression preceeded by unary operators.
    Result<ast::Expr> parse_prefix_expr(TokenTypes sync);

    // Parses suffix expression, i.e. an expression followed by one (or more) function calls,
    // dotted names, function/method calls, index expressions etc.
    Result<ast::Expr> parse_suffix_expr(TokenTypes sync);

    Result<ast::Expr> parse_suffix_expr_inner(std::unique_ptr<ast::Expr> expr);

    // Parses primary expressions (constants, variables, function calls, braced expressions ...)
    Result<ast::Expr> parse_primary_expr(TokenTypes sync);

    // Parses a block expression.
    Result<ast::BlockExpr> parse_block_expr(TokenTypes sync);

    // Parses an if expression.
    Result<ast::IfExpr> parse_if_expr(TokenTypes sync);

    // Returns true if we're at the start of a variable declaration.
    static bool can_begin_var_decl(TokenType type);

    // Returns true if the token type would be a valid start for an expression,
    // e.g. identifiers, literals.
    static bool can_begin_expression(TokenType type);

    // Creates a source reference instance for the given range [begin, end).
    SourceReference ref(size_t begin, size_t end) const;

    template<typename Node>
    Result<Node> result(std::unique_ptr<Node>&& node, bool parse_ok);

    // Returns a failed result that holds the given node. Also makes sure
    // that the node has the error flag set. The node can be null.
    template<typename Node>
    Result<Node> error(std::unique_ptr<Node>&& node);

    // Returns a failed result without a value.
    ErrorTag error();

    // Creates a new result with the given node and the same error flag as `other`.
    template<typename Node, typename OtherNode>
    Result<Node>
    forward(std::unique_ptr<Node>&& node, const Result<OtherNode>& other);

    // Returns the current token if its type is a member of the provided set.
    // Advances the input in that case.
    // Does nothing otherwise.
    std::optional<Token> accept(TokenTypes tokens);

    // Like "accept", but emits an error if the token is of any different type.
    std::optional<Token> expect(TokenTypes tokens);

    // Expects or recovers to the given expected element, depending on the value of "parse_ok".
    std::optional<Token>
    expect_or_recover(bool parse_ok, TokenTypes expected, TokenTypes sync);

    // Forwards to a synchronization token in the `expected` set. Returns true if such
    // a token has been found. Stops if a token in the `sync` set is encountered and
    // returns false in that case.
    bool recover(TokenTypes expected, TokenTypes sync);

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
