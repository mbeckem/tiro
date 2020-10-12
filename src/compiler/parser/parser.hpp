#ifndef TIRO_COMPILER_PARSER_PARSER_HPP
#define TIRO_COMPILER_PARSER_PARSER_HPP

#include "common/adt/function_ref.hpp"
#include "common/adt/span.hpp"
#include "common/text/string_table.hpp"
#include "compiler/ast/ast.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/parser/lexer.hpp"
#include "compiler/parser/parse_result.hpp"
#include "compiler/parser/token.hpp"
#include "compiler/parser/token_types.hpp"
#include "compiler/source_reference.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace tiro {

/// Generates ast node ids.
class AstIdGenerator final {
public:
    AstIdGenerator();

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
    explicit Parser(std::string_view file_name, std::string_view source, StringTable& strings,
        Diagnostics& diag);

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser& parser) = delete;

    Diagnostics& diag() { return diag_; }

    // Parses a file. A file is a sequence of top level items (functions, classes etc.)
    Result<AstFile> parse_file();

    // Parses a toplevel item (e.g. an import or a function declaration).
    Result<AstStmt> parse_item(TokenTypes sync);

private:
    enum ItemFlags {
        HasExport = 1 << 0,
    };

    Result<AstStmt> parse_item_inner(int flags, TokenTypes sync);

    // Parses a list of declaration modifiers.
    Result<AstNodeList<AstModifier>> parse_modifiers(TokenTypes sync);
    Result<AstModifier> parse_modifier(TokenTypes sync);

    // Parses an import declaration.
    Result<AstImportDecl> parse_import_decl(TokenTypes sync);

    // Parses a function declaration.
    Result<AstFuncDecl> parse_func_decl(bool requires_name, TokenTypes sync);

    // Parses a variable declaration.
    Result<AstVarDecl> parse_var_decl(bool with_semicolon, TokenTypes sync);
    Result<AstBinding> parse_binding(bool is_const, TokenTypes sync);
    Result<AstBindingSpec> parse_binding_spec(TokenTypes sync);

public:
    // Parses a single statement.
    Result<AstStmt> parse_stmt(TokenTypes sync);

private:
    Result<AstAssertStmt> parse_assert_stmt(TokenTypes sync);

    // Parses a while loop statement.
    Result<AstWhileStmt> parse_while_stmt(TokenTypes sync);

    // Parses a for loop statement.
    Result<AstForStmt> parse_for_stmt(TokenTypes sync);
    bool parse_for_stmt_header(AstForStmt* stmt, TokenTypes sync);

    // Parses a for each loop statement.
    std::optional<Result<AstForEachStmt>> parse_for_each_stmt(TokenTypes sync);

    Result<AstDeclStmt> parse_var_stmt(TokenTypes sync);

    // Parses a defer statement, e.g. `defer cleanup(args...)`.
    Result<AstDeferStmt> parse_defer_stmt(TokenTypes sync);

    // Parses an expression and wraps it into an expression statement.
    Result<AstExprStmt> parse_expr_stmt(TokenTypes sync);

public:
    // Parses a single expression.
    Result<AstExpr> parse_expr(TokenTypes sync);

private:
    // Recursive parsing function for expressions with infix operators.
    Result<AstExpr> parse_expr(int min_precedence, TokenTypes sync);

    // Parse an expression initiated by an infix operator.
    Result<AstExpr> parse_infix_expr(AstPtr<AstExpr> left, int current_precedence, TokenTypes sync);

    // Parses an expression preceeded by unary operators.
    Result<AstExpr> parse_prefix_expr(TokenTypes sync);

    // Parses "expr.member".
    Result<AstExpr> parse_member_expr(AstPtr<AstExpr> current, TokenTypes sync);

    // Parses expr(args...).
    Result<AstExpr> parse_call_expr(AstPtr<AstExpr> current, TokenTypes sync);

    // Parses expr[args...].
    Result<AstExpr> parse_index_expr(AstPtr<AstExpr> current, TokenTypes sync);

    // Parses primary expressions (constants, variables, function calls, braced expressions ...)
    Result<AstExpr> parse_primary_expr(TokenTypes sync);

    // Parses a plain identifier.
    Result<AstExpr> parse_var_expr(TokenTypes sync);

    // Parses a block expression, i.e. { STMT... }.
    Result<AstExpr> parse_block_expr(TokenTypes sync);

    // Parses an if expression, i.e. if (a) { ... } else { ... }.
    Result<AstExpr> parse_if_expr(TokenTypes sync);

    // Parses a parenthesized expression (either a tuple, a record or a braced expression).
    Result<AstExpr> parse_paren_expr(TokenTypes sync);

    // Parses a tuple literal. The leading "(expr," was already parsed.
    // Note that, because of a previous error, the first_item may be null and will not be
    // made part of the tuple.
    Result<AstExpr> parse_tuple(u32 start, AstPtr<AstExpr> first_item, TokenTypes sync);

    // Parses a record literal. The leading "(identifier: expr" (note: no comma) was already parsed.
    // Note that, because of a previous error, the first_item may be null and will not be
    // made part of the record.
    Result<AstExpr> parse_record(u32 start, AstPtr<AstRecordItem> first_item, TokenTypes sync);

    // Parses a record item, i.e. "key: value_expr".
    // Returns an empty optional if the expected ":" was not found (for backtracking support).
    std::optional<Result<AstRecordItem>> parse_record_item(TokenTypes sync);

    // Parses a group of string literals.
    Result<AstExpr> parse_string_group(TokenTypes sync);

    // Parses a single string expression (literal or interpolated).
    Result<AstStringExpr> parse_string_expr(TokenTypes sync);

    Result<AstExpr> parse_interpolated_expr(TokenType starter, TokenTypes sync);

    // Parses a simple identifier.
    Result<AstStringIdentifier> parse_string_identifier(TokenTypes sync);

    // Parses a property identifier. Switches lexer modes internally to make syntax like `tuple.1` possible.
    Result<AstIdentifier> parse_property_identifier(TokenTypes sync);

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
    auto parse_with_recovery(Parse&& p, Recover&& r);

private:
    // Completes a partially parsed node and returns an error which contains that node.
    template<typename Node>
    Result<Node> partial(AstPtr<Node> node, u32 start) {
        complete_node(node.get(), start, false);
        return syntax_error(std::move(node));
    }

    // Completes a successfully parsed node and returns a successful result that contains that node.
    template<typename Node>
    Result<Node> complete(AstPtr<Node> node, u32 start) {
        complete_node(node.get(), start, true);
        return parse_success(std::move(node));
    }

    // Returns a result for the given node that forwards the error status
    // of the original result. This function is often used for simple wrapper nodes
    // that just contain a single child.
    template<typename Node, typename Original>
    Result<Node> forward(AstPtr<Node> node, u32 start, const Result<Original>& result) {
        return result.is_error() ? partial(std::move(node), start)
                                 : complete(std::move(node), start);
    }

    template<typename Node, typename Position>
    AstPtr<Node> complete_node(AstPtr<Node> node, const Position& pos, bool success) {
        complete_node(node.get(), pos, success);
        return node;
    }

    // Applies start position, id and error flag. Typically the last thing
    // done to a node before construction is complete and the node is returned
    // from the parsing function.
    void complete_node(AstNode* node, u32 start, bool success);
    void complete_node(AstNode* node, const SourceReference& source, bool success);

private:
    /// Returns a reference to the current token. The reference becomes invalid
    /// when advance() is called.
    Token& head();

    /// Advances to the next token. Calling head() will return that token.
    void advance();

    /// Construct a source reference from offsets.
    SourceReference ref(u32 begin, u32 end) const {
        if (end < begin)
            end = begin;
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

    // Backtracking helper
    // FIXME - Bad approach, use hand written backtracking peg parser
    struct StoredPosition {
        Parser* p_;
        size_t pos_;
        size_t messages_;
        std::optional<Token> last_;
        std::optional<Token> head_;

        StoredPosition(Parser* p, size_t pos, size_t messages, std::optional<Token> last,
            std::optional<Token> head)
            : p_(p)
            , pos_(pos)
            , messages_(messages)
            , last_(std::move(last))
            , head_(std::move(head)) {}

        void backtrack() {
            p_->lexer_.pos(pos_);
            p_->diag_.truncate(messages_);
            p_->last_ = last_;
            p_->head_ = head_;
        }
    };

    // Changes the current lexer mode to `mode`. The old lexer mode is restored when the returned
    // guard object is being destroyed.
    ResetLexerMode enter_lexer_mode(LexerMode mode);

    StoredPosition store_position();

    // Returns the start offset of the current token.
    u32 mark_position();

    // Returns the end offset of the last token (if any). Returns the start of the current token otherwise.
    u32 last_position();

private:
    InternedString file_name_;
    std::string_view source_;
    StringTable& strings_;
    Diagnostics& diag_;
    Lexer lexer_;
    AstIdGenerator node_ids_;
    std::optional<Token> last_; // Previous token, updated when advancing
    std::optional<Token> head_; // Buffer for current token - read on demand
};

} // namespace tiro

#endif // TIRO_COMPILER_PARSER_PARSER_HPP
