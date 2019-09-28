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

/**
 * A recursive descent parser.
 *
 * Design notes
 * ============
 *
 * A key design choice in this recursive descent parser is that it handles
 * partially valid nonterminals. The successfully parsed part of a language element
 * is returned on error and the parser attempts to recover from many errors
 * in order to give as many diagnostics as reasonably possible before exiting.
 *
 * Parsing functions for nonterminal language elements usually
 * return a Result<T>. A result instance contains two members:
 *  - Whether the parser is in an OK state (i.e. `parse_ok() == true`). Note that the parser may
 *    be in an OK state even if the returned node contains internal errors (they may have
 *    been recoverable).
 *  - The ast node that was parsed by the function. This node may be null
 *    if `parse_ok()` is false. Otherwise, the node is never null but may contain
 *    internal errors (i.e. `node->has_error() == true`) that the parser was able to recover from.
 *
 * If `parse_ok()` is false, the calling function must attempt recover from the error (e.g. by
 * seeking to the next synchronizing token like ";" or "}") or by forwarding the error to its caller,
 * so it may get handled there. If `parse_ok()` is true, the caller can continue like normal.
 */
class Parser {
public:
    class ErrorTag {};

    // The only logical implication in this class is
    // parse_ok() == true -> has_node() == true.
    template<typename Node>
    class [[nodiscard]] Result {
    public:
        static_assert(std::is_base_of_v<ast::Node, Node>);

        using value_type = std::unique_ptr<Node>;

        // Failure and no node value at all.
        Result(ErrorTag)
            : node_(nullptr)
            , parse_ok_(false) {}

        // Constructs a result. If `parse_ok` is true, the node must not be null.
        template<typename OtherNode,
            std::enable_if_t<std::is_base_of_v<Node, OtherNode>>* = nullptr>
        Result(std::unique_ptr<OtherNode> && node, bool parse_ok = true)
            : node_(std::move(node))
            , parse_ok_(node_ != nullptr && parse_ok) {

            HAMMER_ASSERT(!parse_ok_ || node_ != nullptr,
                "Node must be non-null if parsing succeeded.");
        }

        // Converts the result from a compatible result type.
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

        // True if no parse error occurred. False if the parser must synchronize.
        bool parse_ok() const { return parse_ok_; }

        // If parse_ok() is true, has_node() is always true as well (unless the node has been moved).
        // If parse_ok() is false, has_node() may still be true for partial results.
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
    Result<ast::ImportDecl> parse_import_decl(TokenTypes sync);

    // Parses a function declaration.
    Result<ast::FuncDecl> parse_func_decl(bool requires_name, TokenTypes sync);

public:
    // Parses a single statement.
    Result<ast::Stmt> parse_stmt(TokenTypes sync);

private:
    Result<ast::AssertStmt> parse_assert(TokenTypes sync);

    // Parses a variable / constant declaration.
    // Note: this function does not read up to the ";".
    Result<ast::DeclStmt> parse_var_decl(TokenTypes sync);

    // Parses a while loop statement.
    Result<ast::WhileStmt> parse_while_stmt(TokenTypes sync);

    // Parses a for loop statement.
    Result<ast::ForStmt> parse_for_stmt(TokenTypes sync);
    bool
    parse_for_stmt_header(ast::ForStmt* stmt, bool has_parens, TokenTypes sync);

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
    Result<ast::Expr>
    parse_suffix_expr(std::unique_ptr<ast::Expr> current, TokenTypes sync);
    Result<ast::DotExpr>
    parse_dot_expr(std::unique_ptr<ast::Expr> current, TokenTypes sync);
    Result<ast::CallExpr>
    parse_call_expr(std::unique_ptr<ast::Expr> current, TokenTypes sync);
    Result<ast::IndexExpr>
    parse_index_expr(std::unique_ptr<ast::Expr> current, TokenTypes sync);

    // Parses primary expressions (constants, variables, function calls, braced expressions ...)
    Result<ast::Expr> parse_primary_expr(TokenTypes sync);

    // Parses a block expression, i.e. { STMT... }.
    Result<ast::BlockExpr> parse_block_expr(TokenTypes sync);

    // Parses an if expression, i.e. if (a) { ... } else { ... }.
    Result<ast::IfExpr> parse_if_expr(TokenTypes sync);

    // Parses a parenthesized expression (either a tuple or a braced expression).
    Result<ast::Expr> parse_paren_expr(TokenTypes sync);

    // Parses a tuple literal. The leading "(expr," was already parsed.
    // Note that, because of a previous error, the first_item may be null and will not be
    // made part of the tuple.
    Result<ast::TupleLiteral>
    parse_tuple(std::unique_ptr<ast::Expr> first_item, TokenTypes sync);

    struct ListOptions {
        // Name for error reporting (e.g. "parameter list")
        std::string_view name;
        // Parse until this closing brace. Must set this value.
        TokenType right_brace = TokenType::InvalidToken;
        // Whether to allow a trailing comma before the closing brace or not.
        bool allow_trailing_comma = false;
        // Maximum number of elements, -1 for no limit.
        int max_count = -1;

        constexpr ListOptions(std::string_view name_, TokenType right_brace_)
            : name(name_)
            , right_brace(right_brace_) {}

        constexpr ListOptions& set_allow_trailing_comma(bool allow) {
            this->allow_trailing_comma = allow;
            return *this;
        }

        constexpr ListOptions& set_max_count(int max) {
            this->max_count = max;
            return *this;
        }
    };

    // Parses a braced list of elements.
    // The `parser` argument is invoked for every element until the closing brace has been
    // encountered.
    // Note: the opening brace must have already been read.
    //
    // Returns true if the parser is in an ok state, false otherwise.
    template<typename SubParser>
    bool parse_braced_list(
        const ListOptions& options, TokenTypes sync, SubParser&& parser);

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

    // Forwards to a synchronization token in the `expected` set. Returns true if such
    // a token has been found. Stops if a token in the `sync` set is encountered and
    // returns false in that case.
    bool recover_seek(TokenTypes expected, TokenTypes sync);

    // Like recover_seek(), but also consumes the expected token on success.
    std::optional<Token> recover_consume(TokenTypes expected, TokenTypes sync);

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
