#include "compiler/ast_gen/build_ast.hpp"

#include "compiler/ast/ast.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/syntax/grammar/literals.hpp"
#include "compiler/syntax/syntax_tree.hpp"
#include "compiler/syntax/syntax_type.hpp"
#include "compiler/syntax/token_set.hpp"

#include <optional>
#include <string_view>

namespace tiro::next {

template<typename T, typename... Args>
static NotNull<AstPtr<T>> make_node(Args&&... args);

namespace {

/// A cursor points to a node and allows stateful visitation of its children.
class Cursor final {
public:
    explicit Cursor(SyntaxNodeId id, IndexMapPtr<const SyntaxNode> data)
        : id_(id)
        , data_(std::move(data))
        , children_(data_->children()) {}

    /// Returns the node's id.
    SyntaxNodeId id() const { return id_; }

    /// Returns the syntax type of this node.
    SyntaxType type() const { return data_->type(); }

    /// Returns the node's data.
    auto data() const { return data_; }

    /// Returns the full range of the node's children.
    auto children() const { return children_; }

    /// Returns true if all children of this node have been visited.
    bool at_end() const { return current_child_index_ >= children_.size(); }

    /// Allows access to the node's data via `->`.
    auto operator->() const { return data(); }

    /// Expects the next child to be a token and throws an internal error if that is not the case.
    /// The cursor is advanced on success.
    Token expect_token() { return expect_token(TokenSet::all()); }

    /// Expects the next child to be a token matching `types` and throws an internal error if that is not the case.
    /// The cursor is advanced on success.
    Token expect_token(TokenSet expected) {
        SyntaxChild next = next_child();
        if (next.type() != SyntaxChildType::Token)
            err("expected the next child to be a token");

        auto token = next.as_token();
        if (!expected.contains(token.type()))
            err(fmt::format("unexpected token {}", token));

        advance();
        return token;
    }

    /// Expects the next child to be a node and throws an internal error if that is not the case.
    /// The cursor is advanced on success.
    SyntaxNodeId expect_node() {
        SyntaxChild next = next_child();
        if (next.type() != SyntaxChildType::NodeId)
            err("expected the next child to be a node");
        advance();
        return next.as_node_id();
    }

    /// Expects that the end of this node's children list has been reached.
    void expect_end() const {
        if (!at_end())
            err("unhandled children after the expected end of this node.");
    }

    /// Advances to the next child of this node.
    void advance() {
        if (!at_end())
            ++current_child_index_;
    }

private:
    SyntaxChild next_child() const {
        if (at_end())
            err("no more children in this node");

        return children_[current_child_index_];
    }

    [[noreturn]] void err(const std::string& message) const {
        TIRO_ERROR(
            "Internal error in node of type '{}': {}. This is either a bug in the parser or in the "
            "ast construction algorithm.",
            type(), message);
    }

private:
    SyntaxNodeId id_;
    IndexMapPtr<const SyntaxNode> data_;
    Span<const SyntaxChild> children_;
    size_t current_child_index_ = 0;
};

class AstBuilder {
public:
    explicit AstBuilder(const SyntaxTree& tree, StringTable& strings, Diagnostics& diag)
        : tree_(tree)
        , strings_(strings)
        , diag_(diag) {}

    template<typename Node, typename Func>
    AstPtr<Node> with_syntax_node(Func&& fn);

    NotNull<AstPtr<AstExpr>> build_expr(SyntaxNodeId node_id);

private:
    NotNull<AstPtr<AstExpr>> build_literal(Cursor& c);

private:
    SyntaxNodeId get_syntax_node();
    bool emit_errors(SyntaxNodeId node_id);

    NotNull<AstPtr<AstExpr>> expr_error(SyntaxNodeId node_id);

    Cursor cursor_for(SyntaxNodeId node_id);

    std::string_view source(const Token& token) const {
        return substring(tree_.source(), token.range());
    }

    auto diag_sink(const SourceRange& range) const {
        return [&diag = this->diag_, range](std::string_view error_message) {
            diag.report(Diagnostics::Error, range, std::string(error_message));
        };
    }

private:
    const SyntaxTree& tree_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace

AstPtr<AstNode>
build_program_ast(const SyntaxTree& program_tree, StringTable& strings, Diagnostics& diag);

AstPtr<AstExpr>
build_expr_ast(const SyntaxTree& expr_tree, StringTable& strings, Diagnostics& diag) {
    AstBuilder builder(expr_tree, strings, diag);
    return builder.with_syntax_node<AstExpr>(
        [&](auto node_id) { return builder.build_expr(node_id); });
}

template<typename Node, typename Func>
AstPtr<Node> AstBuilder::with_syntax_node(Func&& fn) {
    SyntaxNodeId node_id = get_syntax_node();
    if (node_id) {
        return fn(node_id);
    }
    return nullptr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_expr(SyntaxNodeId node_id) {
    if (emit_errors(node_id))
        return expr_error(node_id);

    auto c = cursor_for(node_id);
    switch (c.type()) {
    case SyntaxType::Literal:
        return build_literal(c);

    case SyntaxType::ReturnExpr:
    case SyntaxType::ContinueExpr:
    case SyntaxType::BreakExpr:
    case SyntaxType::VarExpr:
    case SyntaxType::UnaryExpr:
    case SyntaxType::BinaryExpr:
    case SyntaxType::MemberExpr:
    case SyntaxType::IndexExpr:
    case SyntaxType::CallExpr:
    case SyntaxType::ConstructExpr:
    case SyntaxType::GroupedExpr:
    case SyntaxType::TupleExpr:
    case SyntaxType::RecordExpr:
    case SyntaxType::ArrayExpr:
    case SyntaxType::IfExpr:
    case SyntaxType::BlockExpr:
    case SyntaxType::FuncExpr:
    case SyntaxType::StringExpr:
    default:
        TIRO_ERROR("unexpected syntax type {} in expression context", c.type());
    }
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_literal(Cursor& c) {
    auto token = c.expect_token({
        TokenType::Integer,
        TokenType::Float,
        TokenType::Symbol,
        TokenType::KwTrue,
        TokenType::KwFalse,
        TokenType::KwNull,
    });

    switch (token.type()) {
    case TokenType::KwTrue:
        return make_node<AstBooleanLiteral>(true);
    case TokenType::KwFalse:
        return make_node<AstBooleanLiteral>(false);
    case TokenType::KwNull:
        return make_node<AstNullLiteral>();
    case TokenType::Symbol: {
        auto name = parse_symbol_name(source(token), diag_sink(token.range()));
        if (!name)
            return expr_error(c.id());
        return make_node<AstSymbolLiteral>(strings_.insert(*name));
    }
    case TokenType::Integer: {
        auto value = parse_integer_value(source(token), diag_sink(token.range()));
        if (!value)
            return expr_error(c.id());
        return make_node<AstIntegerLiteral>(*value);
    }
    case TokenType::Float: {
        auto value = parse_float_value(source(token), diag_sink(token.range()));
        if (!value)
            return expr_error(c.id());
        return make_node<AstFloatLiteral>(*value);
    }
    default:
        TIRO_ERROR("unexpected token type {} in literal context", token.type());
    }
}

SyntaxNodeId AstBuilder::get_syntax_node() {
    // Visit the root node and emit errors.
    // The root node contains errors that could not be attached to any open syntax node during parsing.
    auto root_id = tree_.root_id();
    TIRO_DEBUG_ASSERT(root_id, "Syntax tree does not have a root.");
    if (emit_errors(root_id))
        return {};

    auto root_data = tree_[root_id];
    TIRO_DEBUG_ASSERT(
        root_data->type() == SyntaxType::Root, "Root node must have syntax type 'Root'.");
    TIRO_DEBUG_ASSERT(
        root_data->children().size() <= 1, "Root node should have at most one child.");

    if (root_data->children().empty())
        return {};

    auto child = root_data->children().front();
    TIRO_DEBUG_ASSERT(child.type() == SyntaxChildType::NodeId,
        "Children of the root node must be nodes as well.");
    return child.as_node_id();
}

bool AstBuilder::emit_errors(SyntaxNodeId node_id) {
    auto node_data = tree_[node_id];
    bool is_error = false;
    // FIXME diagnostics must use SourceRange instead!
    SourceReference ref(node_data->range().begin(), node_data->range().end());

    for (const auto& error : node_data->errors()) {
        diag_.report(Diagnostics::Error, ref, error);
        is_error = true;
    }

    if (node_data->type() == SyntaxType::Error) {
        diag_.report(Diagnostics::Error, ref, "syntax error");
        is_error = true;
    }
    return is_error;
}

NotNull<AstPtr<AstExpr>> AstBuilder::expr_error(SyntaxNodeId node_id) {
    // FIXME Build exprerror instance
    (void) node_id;
    TIRO_ERROR("Error expression");
}

Cursor AstBuilder::cursor_for(SyntaxNodeId node_id) {
    return Cursor(node_id, tree_[node_id]);
}

template<typename T, typename... Args>
NotNull<AstPtr<T>> make_node(Args&&... args) {
    return TIRO_NN(std::make_unique<T>(std::forward<Args>(args)...));
}

} // namespace tiro::next
