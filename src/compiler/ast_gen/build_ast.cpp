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

static std::optional<UnaryOperator> to_unary_operator(TokenType t);
static std::optional<BinaryOperator> to_binary_operator(TokenType t);

[[noreturn]] static void err(SyntaxType node_type, std::string_view message) {
    TIRO_ERROR(
        "In node of type '{}': {}. This is either a bug in the parser or in "
        "the ast construction algorithm.",
        node_type, message);
}

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
    auto operator-> () const { return data(); }

    /// Expects the next child to be a token and throws an internal error if that is not the case.
    /// The cursor is advanced on success.
    Token expect_token() { return expect_token(TokenSet::all()); }

    /// Expects the next child to be a token matching `types` and throws an internal error if that is not the case.
    /// The cursor is advanced on success.
    Token expect_token(TokenSet expected) {
        SyntaxChild next = next_child();
        if (next.type() != SyntaxChildType::Token)
            err(type(), "expected the next child to be a token");

        auto token = next.as_token();
        if (!expected.contains(token.type()))
            err(type(), fmt::format("unexpected token {}", token));

        advance();
        return token;
    }

    /// Advances to the next child if `at_token(expected)` returns true.
    std::optional<Token> accept_token(TokenSet expected) {
        if (at_token(expected)) {
            auto token = next_child().as_token();
            advance();
            return token;
        }
        return {};
    }

    /// Returns true if the next child is a token and matches the given token set.
    bool at_token(TokenSet expected) const {
        if (at_end())
            return false;

        SyntaxChild next = next_child();
        return next.type() == SyntaxChildType::Token && expected.contains(next.as_token().type());
    }

    /// Expects the next child to be a node and throws an internal error if that is not the case.
    /// The cursor is advanced on success.
    SyntaxNodeId expect_node() {
        SyntaxChild next = next_child();
        if (next.type() != SyntaxChildType::NodeId)
            err(type(), "expected the next child to be a node");
        advance();
        return next.as_node_id();
    }

    /// Expects that the end of this node's children list has been reached.
    void expect_end() const {
        if (!at_end())
            err(type(), "unexpected children after the expected end of this node");
    }

    /// Advances to the next child of this node.
    void advance() {
        if (!at_end())
            ++current_child_index_;
    }

private:
    SyntaxChild next_child() const {
        if (at_end())
            err(type(), "no more children in this node");

        return children_[current_child_index_];
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
    NotNull<AstPtr<AstExpr>> build_group(Cursor& c);
    NotNull<AstPtr<AstExpr>> build_return(Cursor& c);
    NotNull<AstPtr<AstExpr>> build_member(Cursor& c);
    NotNull<AstPtr<AstExpr>> build_index(Cursor& c);
    NotNull<AstPtr<AstExpr>> build_binary(Cursor& c);
    NotNull<AstPtr<AstExpr>> build_unary(Cursor& c);
    NotNull<AstPtr<AstExpr>> build_array(Cursor& c);
    NotNull<AstPtr<AstExpr>> build_tuple(Cursor& c);
    NotNull<AstPtr<AstExpr>> build_string(Cursor& c);
    NotNull<AstPtr<AstExpr>> build_string_group(Cursor& c);

    void gather_string_contents(AstNodeList<AstExpr>& items, Cursor& c);

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
    std::string buffer_;
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
    case SyntaxType::VarExpr: {
        auto ident = c.expect_token(TokenType::Identifier);
        c.expect_end();
        return make_node<AstVarExpr>(strings_.insert(source(ident)));
    }
    case SyntaxType::Literal:
        return build_literal(c);
    case SyntaxType::GroupedExpr:
        return build_group(c);
    case SyntaxType::ContinueExpr:
        c.expect_token(TokenType::KwContinue);
        c.expect_end();
        return make_node<AstContinueExpr>();
    case SyntaxType::BreakExpr:
        c.expect_token(TokenType::KwBreak);
        c.expect_end();
        return make_node<AstBreakExpr>();
    case SyntaxType::MemberExpr:
        return build_member(c);
    case SyntaxType::IndexExpr:
        return build_index(c);
    case SyntaxType::ReturnExpr:
        return build_return(c);
    case SyntaxType::BinaryExpr:
        return build_binary(c);
    case SyntaxType::UnaryExpr:
        return build_unary(c);
    case SyntaxType::ArrayExpr:
        return build_array(c);
    case SyntaxType::TupleExpr:
        return build_tuple(c);
    case SyntaxType::StringExpr:
        return build_string(c);
    case SyntaxType::StringGroup:
        return build_string_group(c);

    case SyntaxType::CallExpr:
    case SyntaxType::ConstructExpr:
    case SyntaxType::RecordExpr:
    case SyntaxType::IfExpr:
    case SyntaxType::BlockExpr:
    case SyntaxType::FuncExpr:
    default:
        err(c.type(), "syntax type is not supported in expression context");
    }
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_literal(Cursor& c) {
    auto token = c.expect_token({
        TokenType::KwTrue,
        TokenType::KwFalse,
        TokenType::KwNull,
        TokenType::Symbol,
        TokenType::Integer,
        TokenType::Float,
    });
    c.expect_end();

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
        TIRO_UNREACHABLE("unhandled token type in literal expression");
    }
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_group(Cursor& c) {
    c.expect_token(TokenType::LeftParen);
    auto expr = build_expr(c.expect_node());
    c.expect_token(TokenType::RightParen);
    c.expect_end();
    return expr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_return(Cursor& c) {
    c.expect_token(TokenType::KwReturn);

    AstPtr<AstExpr> inner;
    if (!c.at_end()) {
        inner = build_expr(c.expect_node()).get();
    }
    c.expect_end();

    auto expr = make_node<AstReturnExpr>();
    expr->value(std::move(inner));
    return expr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_member(Cursor& c) {
    auto build_property = [&](SyntaxNodeId member_id) -> AstPtr<AstIdentifier> {
        if (emit_errors(member_id))
            return {};

        auto member_c = cursor_for(member_id);
        auto name = member_c.expect_token({TokenType::Identifier, TokenType::TupleField});
        member_c.expect_end();

        switch (name.type()) {
        case TokenType::Identifier:
            return make_node<AstStringIdentifier>(strings_.insert(source(name)));
        case TokenType::TupleField: {
            auto index = parse_tuple_field(source(name), diag_sink(name.range()));
            if (!index)
                return nullptr;
            return make_node<AstNumericIdentifier>(*index);
        }
        default:
            TIRO_UNREACHABLE("unhandled token type in member name");
        }
    };

    auto instance = build_expr(c.expect_node());
    auto access = c.expect_token({TokenType::Dot, TokenType::QuestionDot});
    auto property = build_property(c.expect_node());
    c.expect_end();

    if (!property)
        return expr_error(c.id());

    auto node = make_node<AstPropertyExpr>(
        access.type() == TokenType::QuestionDot ? AccessType::Optional : AccessType::Normal);
    node->instance(std::move(instance));
    node->property(std::move(property));
    return node;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_index(Cursor& c) {
    auto instance = build_expr(c.expect_node());
    auto open_bracket = c.expect_token({TokenType::QuestionLeftBracket, TokenType::LeftBracket});
    auto element = build_expr(c.expect_node());
    c.expect_token(TokenType::RightBracket);
    c.expect_end();

    auto node = make_node<AstElementExpr>(open_bracket.type() == TokenType::QuestionLeftBracket
                                              ? AccessType::Optional
                                              : AccessType::Normal);
    node->instance(std::move(instance));
    node->element(std::move(element));
    return node;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_binary(Cursor& c) {
    auto lhs = build_expr(c.expect_node());
    auto op_token = c.expect_token();
    auto rhs = build_expr(c.expect_node());
    c.expect_end();

    auto op = to_binary_operator(op_token.type());
    if (!op)
        err(c.type(), fmt::format("unexpected binary operator {}", op_token));

    auto node = make_node<AstBinaryExpr>(*op);
    node->left(std::move(lhs));
    node->right(std::move(rhs));
    return node;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_unary(Cursor& c) {
    auto op_token = c.expect_token();
    auto expr = build_expr(c.expect_node());
    c.expect_end();

    auto op = to_unary_operator(op_token.type());
    if (!op)
        err(c.type(), fmt::format("unexpected unary operator {}", op_token));

    auto node = make_node<AstUnaryExpr>(*op);
    node->inner(std::move(expr));
    return node;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_array(Cursor& c) {
    AstNodeList<AstExpr> items;

    c.expect_token(TokenType::LeftBracket);
    while (!c.at_end() && !c.at_token(TokenType::RightBracket)) {
        items.append(build_expr(c.expect_node()));
        if (!c.accept_token(TokenType::Comma))
            break;
    }
    c.expect_token(TokenType::RightBracket);
    c.expect_end();

    auto array = make_node<AstArrayLiteral>();
    array->items(std::move(items));
    return array;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_tuple(Cursor& c) {
    AstNodeList<AstExpr> items;

    c.expect_token(TokenType::LeftParen);
    while (!c.at_end() && !c.at_token(TokenType::RightParen)) {
        items.append(build_expr(c.expect_node()));
        if (!c.accept_token(TokenType::Comma))
            break;
    }
    c.expect_token(TokenType::RightParen);
    c.expect_end();

    auto array = make_node<AstTupleLiteral>();
    array->items(std::move(items));
    return array;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_string(Cursor& c) {
    AstNodeList<AstExpr> items;
    gather_string_contents(items, c);

    auto string = make_node<AstStringExpr>();
    string->items(std::move(items));
    return string;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_string_group(Cursor& c) {
    AstNodeList<AstExpr> items;
    while (!c.at_end()) {
        auto string_cursor = cursor_for(c.expect_node());
        if (string_cursor.type() != SyntaxType::StringExpr)
            err(string_cursor.type(), "invalid item type in string group");
        gather_string_contents(items, string_cursor);
    }
    c.expect_end();

    auto string = make_node<AstStringExpr>();
    string->items(std::move(items));
    return string;
}

void AstBuilder::gather_string_contents(AstNodeList<AstExpr>& items, Cursor& c) {
    c.expect_token(TokenType::StringStart);
    while (!c.at_end() && !c.at_token(TokenType::StringEnd)) {
        if (auto content = c.accept_token(TokenType::StringContent)) {
            buffer_.clear();
            parse_string_literal(source(*content), buffer_, diag_sink(content->range()));
            items.append(make_node<AstStringLiteral>(strings_.insert(buffer_)));
            continue;
        }

        auto item_cursor = cursor_for(c.expect_node());
        switch (item_cursor->type()) {
        case SyntaxType::StringFormatItem:
            item_cursor.expect_token(TokenType::StringVar);
            items.append(build_expr(item_cursor.expect_node()));
            item_cursor.expect_end();
            break;
        case SyntaxType::StringFormatBlock:
            item_cursor.expect_token(TokenType::StringBlockStart);
            items.append(build_expr(item_cursor.expect_node()));
            item_cursor.expect_token(TokenType::StringBlockEnd);
            item_cursor.expect_end();
            break;
        default:
            err(item_cursor.type(), "invalid string item type");
        }
    }
    c.expect_token(TokenType::StringEnd);
    c.expect_end();
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

std::optional<UnaryOperator> to_unary_operator(TokenType t) {
    switch (t) {
    case TokenType::Plus:
        return UnaryOperator::Plus;
    case TokenType::Minus:
        return UnaryOperator::Minus;
    case TokenType::LogicalNot:
        return UnaryOperator::LogicalNot;
    case TokenType::BitwiseNot:
        return UnaryOperator::BitwiseNot;
    default:
        return {};
    }
}

std::optional<BinaryOperator> to_binary_operator(TokenType t) {
#define TIRO_MAP_TOKEN(token, op) \
    case TokenType::token:        \
        return BinaryOperator::op;

    switch (t) {
        TIRO_MAP_TOKEN(Plus, Plus)
        TIRO_MAP_TOKEN(Minus, Minus)
        TIRO_MAP_TOKEN(Star, Multiply)
        TIRO_MAP_TOKEN(Slash, Divide)
        TIRO_MAP_TOKEN(Percent, Modulus)
        TIRO_MAP_TOKEN(StarStar, Power)
        TIRO_MAP_TOKEN(LeftShift, LeftShift)
        TIRO_MAP_TOKEN(RightShift, RightShift)

        TIRO_MAP_TOKEN(BitwiseAnd, BitwiseAnd)
        TIRO_MAP_TOKEN(BitwiseOr, BitwiseOr)
        TIRO_MAP_TOKEN(BitwiseXor, BitwiseXor)

        TIRO_MAP_TOKEN(Less, Less)
        TIRO_MAP_TOKEN(LessEquals, LessEquals)
        TIRO_MAP_TOKEN(Greater, Greater)
        TIRO_MAP_TOKEN(GreaterEquals, GreaterEquals)
        TIRO_MAP_TOKEN(EqualsEquals, Equals)
        TIRO_MAP_TOKEN(NotEquals, NotEquals)
        TIRO_MAP_TOKEN(LogicalAnd, LogicalAnd)
        TIRO_MAP_TOKEN(LogicalOr, LogicalOr)
        TIRO_MAP_TOKEN(QuestionQuestion, NullCoalesce)

        TIRO_MAP_TOKEN(Equals, Assign)
        TIRO_MAP_TOKEN(PlusEquals, AssignPlus)
        TIRO_MAP_TOKEN(MinusEquals, AssignMinus)
        TIRO_MAP_TOKEN(StarEquals, AssignMultiply)
        TIRO_MAP_TOKEN(StarStarEquals, AssignPower)
        TIRO_MAP_TOKEN(SlashEquals, AssignDivide)
        TIRO_MAP_TOKEN(PercentEquals, AssignModulus)

    default:
        return {};
    }

#undef TIRO_MAP_TOKEN
}

} // namespace tiro::next
