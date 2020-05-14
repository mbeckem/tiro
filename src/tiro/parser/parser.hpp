#ifndef TIRO_PARSER_PARSER_HPP
#define TIRO_PARSER_PARSER_HPP

#include "tiro/ast/ast.hpp"
#include "tiro/ast/token.hpp"
#include "tiro/ast/token_types.hpp"
#include "tiro/compiler/diagnostics.hpp"
#include "tiro/compiler/source_reference.hpp"
#include "tiro/core/function_ref.hpp"
#include "tiro/core/span.hpp"
#include "tiro/core/string_table.hpp"
#include "tiro/parser/lexer.hpp"
#include "tiro/parser/parse_result.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace tiro {

/// Generates ast node ids.
class AstIds final {
public:
    AstIds();

    AstId generate();

private:
    u32 next_id_;
};

/// A recursive descent parser.
///
/// A key design choice in this recursive descent parser is that it handles
/// partially valid nonterminals. The successfully parsed part of a language element
/// is returned on error and the parser attempts to recover from many errors
/// in order to give as many diagnostics as reasonably possible before exiting.
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
    Result<AstFile> parse_file();

    // Parses a toplevel item (e.g. an import or a function declaration).
    Result<AstItem> parse_toplevel_item(TokenTypes sync);

private:
    // Parses an import declaration.
    Result<AstItem> parse_import_decl(TokenTypes sync);

    // Parses a function declaration.
    Result<AstFuncDecl> parse_func_decl(bool requires_name, TokenTypes sync);

    // Parses a variable declarations.
    // Note: this function does not read the final ";".
    Result<AstItem> parse_var_decl(TokenTypes sync);
    Result<AstBinding> parse_binding(bool is_const, TokenTypes sync);
    Result<AstBinding> parse_binding_lhs(bool is_const, TokenTypes sync);

public:
    // Parses a single statement.
    Result<AstStmt> parse_stmt(TokenTypes sync);

private:
    Result<AstStmt> parse_assert(TokenTypes sync);

    Result<AstStmt> parse_decl_stmt(TokenTypes sync);

    // Parses a while loop statement.
    Result<AstStmt> parse_while_stmt(TokenTypes sync);

    // Parses a for loop statement.
    Result<AstStmt> parse_for_stmt(TokenTypes sync);
    bool parse_for_stmt_header(AstStmt* stmt, TokenTypes sync);

    // Parses an expression and wraps it into an expression statement.
    Result<AstStmt> parse_expr_stmt(TokenTypes sync);

public:
    // Parses a single expression.
    Result<AstExpr> parse_expr(TokenTypes sync);

private:
    // Recursive parsing function for expressions with infix operators.
    Result<AstExpr> parse_expr(int min_precedence, TokenTypes sync);

    // Parse an expression initiated by an infix operator.
    Result<AstExpr>
    parse_infix_expr(AstExpr left, int current_precedence, TokenTypes sync);

    // Parses "expr.member".
    Result<AstExpr> parse_member_expr(AstExpr current, TokenTypes sync);

    // Parses expr(args...).
    Result<AstExpr> parse_call_expr(AstExpr current, TokenTypes sync);

    // Parses expr[args...].
    Result<AstExpr> parse_index_expr(AstExpr* current, TokenTypes sync);

    // Parses an expression preceeded by unary operators.
    Result<AstExpr> parse_prefix_expr(TokenTypes sync);

    // Parses primary expressions (constants, variables, function calls, braced expressions ...)
    Result<AstExpr> parse_primary_expr(TokenTypes sync);

    // Parses a plain identifier.
    Result<AstExpr> parse_identifier(TokenTypes sync);

    // Parses a block expression, i.e. { STMT... }.
    Result<AstExpr> parse_block_expr(TokenTypes sync);

    // Parses an if expression, i.e. if (a) { ... } else { ... }.
    Result<AstExpr> parse_if_expr(TokenTypes sync);

    // Parses a parenthesized expression (either a tuple or a braced expression).
    Result<AstExpr> parse_paren_expr(TokenTypes sync);

    // Parses a tuple literal. The leading "(expr," was already parsed.
    // Note that, because of a previous error, the first_item may be null and will not be
    // made part of the tuple.
    Result<AstExpr>
    parse_tuple(const Token& start_tok, AstExpr first_item, TokenTypes sync);

    // Parses a group of string literals.
    Result<AstExpr> parse_string_sequence(TokenTypes sync);

    // Parses a single string expression (literal or interpolated).
    Result<AstExpr> parse_string_expr(TokenTypes sync);

    Result<AstExpr> parse_interpolated_expr(TokenType starter, TokenTypes sync);

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

    template<typename Parse, typename Recover>
    auto invoke(Parse&& p, Recover&& r);

private:
    AstId next_id();

private:
    /// Returns a reference to the current token. The reference becomes invalid
    /// when advance() is called.
    Token& head();

    /// Advances to the next token. Calling head() will return that token.
    void advance();

    /// Construct a source reference from offsets.
    SourceReference ref(u32 begin, u32 end) const {
        return SourceReference(file_name_, begin, end);
    }

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

    struct SourceRangeTracker {
        Parser* p_;
        u32 begin_;

        SourceRangeTracker(Parser* p, u32 begin)
            : p_(p)
            , begin_(begin) {}

        SourceReference finish() {
            // Source ranges for ast nodes usually end with the last token
            // that was accepted as part of that node.
            u32 end = p_->last_ ? p_->last_->source().end() : begin_;
            return p_->ref(begin_, end);
        }
    };

    // Start a source range with the start of the current head token.
    SourceRangeTracker begin_range();

    // Start a source range with the given offset.
    SourceRangeTracker begin_range(u32 begin);

private:
    InternedString file_name_;
    std::string_view source_;
    StringTable& strings_;
    Diagnostics& diag_;
    Lexer lexer_;
    AstIds node_ids_;
    std::optional<Token> last_; // Previous token, updated when advancing
    std::optional<Token> head_; // Buffer for current token - read on demand
};

} // namespace tiro

#endif // TIRO_PARSER_PARSER_HPP