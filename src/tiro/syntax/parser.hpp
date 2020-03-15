#ifndef TIRO_SYNTAX_PARSER_HPP
#define TIRO_SYNTAX_PARSER_HPP

#include "tiro/compiler/diagnostics.hpp"
#include "tiro/compiler/source_reference.hpp"
#include "tiro/core/function_ref.hpp"
#include "tiro/core/span.hpp"
#include "tiro/core/string_table.hpp"
#include "tiro/syntax/ast.hpp"
#include "tiro/syntax/lexer.hpp"
#include "tiro/syntax/parse_result.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace tiro::compiler {

///  A recursive descent parser.
///
///  Design notes
///  ============
///
///  A key design choice in this recursive descent parser is that it handles
///  partially valid nonterminals. The successfully parsed part of a language element
///  is returned on error and the parser attempts to recover from many errors
///  in order to give as many diagnostics as reasonably possible before exiting.
///
///  Parsing functions for nonterminal language elements usually
///  return a Result<T>. A result instance contains two members:
///   - Whether the parser is in an OK state (i.e. `parse_ok() == true`). Note that the parser may
///     be in an OK state even if the returned node contains internal errors (they may have
///     been recoverable).
///   - The ast node that was parsed by the function. This node may be null
///     if `parse_ok()` is false. Otherwise, the node is never null but may contain
///     internal errors (i.e. `node->has_error() == true`) that the parser was able to recover from.
///
///  If `parse_ok()` is false, the calling function must attempt recover from the error (e.g. by
///  seeking to the next synchronizing token like ";" or "}") or by forwarding the error to its caller,
///  so it may get handled there. If `parse_ok()` is true, the caller can continue like normal.
class Parser final {
public:
    template<typename NodeT>
    using Result = ParseResult<NodeT>;

public:
    explicit Parser(std::string_view file_name, std::string_view source,
        StringTable& strings, Diagnostics& diag);

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser& parser) = delete;

    Diagnostics& diag() { return diag_; }

    // Parses a file. A file is a sequence of top level items (functions, classes etc.)
    Result<File> parse_file();

    // Parses a toplevel item (e.g. an import or a function declaration).
    Result<Node> parse_toplevel_item(TokenTypes sync);

private:
    // Parses an import declaration.
    Result<ImportDecl> parse_import_decl(TokenTypes sync);

    // Parses a function declaration.
    Result<FuncDecl> parse_func_decl(bool requires_name, TokenTypes sync);

public:
    // Parses a single statement.
    Result<Stmt> parse_stmt(TokenTypes sync);

private:
    Result<AssertStmt> parse_assert(TokenTypes sync);

    Result<DeclStmt> parse_decl_stmt(TokenTypes sync);

    // Parses a variable / constant declaration.
    // Note: this function does not read up to the ";".
    Result<DeclStmt> parse_var_decl(TokenTypes sync);
    Result<Binding> parse_binding(bool is_const, TokenTypes sync);
    Result<Binding> parse_binding_lhs(bool is_const, TokenTypes sync);

    // Parses a while loop statement.
    Result<WhileStmt> parse_while_stmt(TokenTypes sync);

    // Parses a for loop statement.
    Result<ForStmt> parse_for_stmt(TokenTypes sync);
    bool parse_for_stmt_header(ForStmt* stmt, TokenTypes sync);

    // Parses an expression and wraps it into an expression statement.
    Result<ExprStmt> parse_expr_stmt(TokenTypes sync);

public:
    // Parses an expression. Public for testing.
    Result<Expr> parse_expr(TokenTypes sync);

private:
    // Recursive parsing function for expressions with infix operators.
    Result<Expr> parse_expr(int min_precedence, TokenTypes sync);

    // Parse an expression initiated by an infix operator.
    Result<Expr>
    parse_infix_expr(Expr* left, int current_precedence, TokenTypes sync);

    // Parses "expr.member".
    Result<Expr> parse_member_expr(Expr* current, TokenTypes sync);

    // Parses expr(args...).
    Result<CallExpr> parse_call_expr(Expr* current, TokenTypes sync);

    // Parses expr[args...].
    Result<IndexExpr> parse_index_expr(Expr* current, TokenTypes sync);

    // Parses an expression preceeded by unary operators.
    Result<Expr> parse_prefix_expr(TokenTypes sync);

    // Parses primary expressions (constants, variables, function calls, braced expressions ...)
    Result<Expr> parse_primary_expr(TokenTypes sync);

    // Parses a plain identifier.
    Result<VarExpr> parse_identifier(TokenTypes sync);

    // Parses a block expression, i.e. { STMT... }.
    Result<BlockExpr> parse_block_expr(TokenTypes sync);

    // Parses an if expression, i.e. if (a) { ... } else { ... }.
    Result<IfExpr> parse_if_expr(TokenTypes sync);

    // Parses a parenthesized expression (either a tuple or a braced expression).
    Result<Expr> parse_paren_expr(TokenTypes sync);

    // Parses a tuple literal. The leading "(expr," was already parsed.
    // Note that, because of a previous error, the first_item may be null and will not be
    // made part of the tuple.
    Result<TupleLiteral>
    parse_tuple(const Token& start_tok, Expr* first_item, TokenTypes sync);

    // Parses a group of string literals.
    Result<Expr> parse_string_sequence(TokenTypes sync);

    // Parses a single string expression (literal or interpolated).
    Result<Expr> parse_string_expr(TokenTypes sync);

    Result<Expr> parse_interpolated_expr(TokenType starter, TokenTypes sync);

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
    bool parse_braced_list(const ListOptions& options, TokenTypes sync,
        FunctionRef<bool(TokenTypes inner_sync)> parser);

    template<typename Node, typename... Args>
    NodePtr<Node> make_node(const Token& start, Args&&... args);

    template<typename Node>
    Result<Node> result(NodePtr<Node>&& node, bool parse_ok);

    // Returns a failed result that holds the given node. Also makes sure
    // that the node has the error flag set. The node can be null.
    template<typename Node>
    Result<Node> error(NodePtr<Node>&& node);

    // Creates a new result with the given node and the same error flag as `other`.
    template<typename Node, typename OtherNode>
    Result<Node> forward(NodePtr<Node>&& node, const Result<OtherNode>& other);

    template<typename Parse, typename Recover>
    auto invoke(Parse&& p, Recover&& r);

    /// Returns a reference to the current token. The reference becomes invalid
    /// when advance() is called.
    Token& head();

    /// Advances to the next token. Calling head() will return that token.
    void advance();

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

    struct [[nodiscard]] ResetLexerMode {
        Parser* p_;
        LexerMode m_;

        ResetLexerMode(Parser * p, LexerMode m)
            : p_(p)
            , m_(m) {}

        ~ResetLexerMode() {
            if (p_)
                p_->lexer_.mode(m_);
        };

        ResetLexerMode(const ResetLexerMode&) = delete;
        ResetLexerMode& operator=(const ResetLexerMode&) = delete;
    };

    ResetLexerMode enter_lexer_mode(LexerMode mode);

private:
    InternedString file_name_;
    std::string_view source_;
    StringTable& strings_;
    Diagnostics& diag_;
    Lexer lexer_;
    std::optional<Token> head_; // Buffer for current token - read on demand
};

} // namespace tiro::compiler

#endif // TIRO_SYNTAX_PARSER_HPP
