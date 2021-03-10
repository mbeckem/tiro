#include "compiler/ast_gen/build_ast.hpp"

#include "common/fix.hpp"
#include "compiler/ast/ast.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/syntax/grammar/literals.hpp"
#include "compiler/syntax/syntax_tree.hpp"
#include "compiler/syntax/syntax_type.hpp"
#include "compiler/syntax/token_set.hpp"

#include <optional>
#include <string_view>

namespace tiro::next {

static void emit_errors(const SyntaxTree& tree, Diagnostics& diag);

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
    explicit Cursor(const SyntaxTree& tree, SyntaxNodeId id, IndexMapPtr<const SyntaxNode> data)
        : tree_(tree)
        , id_(id)
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

    std::optional<SyntaxNodeId> at_node(SyntaxType expected) const {
        if (at_end())
            return {};

        SyntaxChild next = next_child();
        if (next.type() != SyntaxChildType::NodeId)
            return {};

        auto id = next.as_node_id();
        auto data = tree_[id];
        if (data->type() != expected)
            return {};

        return id;
    }

    std::optional<SyntaxNodeId> accept_node(SyntaxType expected) {
        if (auto node_id = at_node(expected)) {
            advance();
            return node_id;
        }
        return {};
    }

    SyntaxNodeId expect_node(SyntaxType expected) {
        if (auto node_id = accept_node(expected))
            return *node_id;

        err(type(), fmt::format("expected the next child to be a node of type {}", expected));
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

    /// Returns the source range of the next child, or the parent's range as a fallback.
    SourceRange next_range() const {
        if (at_end())
            return data_->range();

        auto child = next_child();
        switch (child.type()) {
        case SyntaxChildType::Token:
            return child.as_token().range();
        case SyntaxChildType::NodeId:
            return tree_[child.as_node_id()]->range();
        }

        TIRO_UNREACHABLE("invalid child type");
    }

private:
    SyntaxChild next_child() const {
        if (at_end())
            err(type(), "no more children in this node");

        return children_[current_child_index_];
    }

private:
    const SyntaxTree& tree_;
    SyntaxNodeId id_;
    IndexMapPtr<const SyntaxNode> data_;
    Span<const SyntaxChild> children_;
    size_t current_child_index_ = 0;
};

/// Implements the syntax tree -> abstract syntax tree transformation.
class AstBuilder {
public:
    explicit AstBuilder(const SyntaxTree& tree, StringTable& strings, Diagnostics& diag)
        : tree_(tree)
        , strings_(strings)
        , diag_(diag) {}

    template<typename Node, typename Func>
    AstPtr<Node> build(Func&& fn);

    NotNull<AstPtr<AstStmt>> build_stmt(SyntaxNodeId node_id) {
        auto maybe_stmt = cursor_for(node_id);
        if (!maybe_stmt)
            return stmt_error(node_id);
        return build_stmt(*maybe_stmt);
    }

    NotNull<AstPtr<AstExpr>> build_expr(SyntaxNodeId node_id) {
        auto maybe_expr = cursor_for(node_id);
        if (!maybe_expr)
            return expr_error(node_id);
        return build_expr(*maybe_expr);
    }

private:
    NotNull<AstPtr<AstStmt>> build_stmt(Cursor& c);

    NotNull<AstPtr<AstExpr>> build_expr(Cursor& c);
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
    NotNull<AstPtr<AstExpr>> build_if(Cursor& c);
    NotNull<AstPtr<AstExpr>> build_block(Cursor& c);
    NotNull<AstPtr<AstExpr>> build_func(Cursor& c);
    NotNull<AstPtr<AstExpr>> build_call(Cursor& c);
    NotNull<AstPtr<AstExpr>> build_construct(Cursor& c);

    AstPtr<AstExpr> build_cond(SyntaxNodeId id);
    AstPtr<AstFuncDecl> build_func_decl(SyntaxNodeId id);
    std::optional<std::string_view> build_name(SyntaxNodeId id);

    std::optional<std::tuple<AccessType, AstNodeList<AstExpr>>> build_args(SyntaxNodeId args_id);

    void gather_string_contents(AstNodeList<AstExpr>& items, Cursor& c);
    void gather_params(AstNodeList<AstParamDecl>& params, Cursor& c);
    void gather_modifiers(AstNodeList<AstModifier>& modifiers, Cursor& c);

    std::optional<AstNodeList<AstMapItem>> gather_map_items(Cursor& c);
    std::optional<AstNodeList<AstExpr>> gather_set_items(Cursor& c);

private:
    // Returns the topmost syntax node (direct child of the root) or an invalid id if
    // the root contains errors.
    SyntaxNodeId get_syntax_node();

    NotNull<AstPtr<AstExpr>> expr_error(SyntaxNodeId node_id);
    NotNull<AstPtr<AstStmt>> stmt_error(SyntaxNodeId node_id);

    // Returns a cursor for the given node.
    // Only returns a valid cursor if the node does not contain any direct errors.
    // Otherwise, an empty optional is returned to signal that this node should be skipped.
    std::optional<Cursor> cursor_for(SyntaxNodeId node_id);

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
    return builder.build<AstExpr>([&](auto node_id) { return builder.build_expr(node_id); });
}

template<typename Node, typename Func>
AstPtr<Node> AstBuilder::build(Func&& fn) {
    emit_errors(tree_, diag_);

    SyntaxNodeId node_id = get_syntax_node();
    if (node_id) {
        return fn(node_id);
    }
    return nullptr;
}

NotNull<AstPtr<AstStmt>> AstBuilder::build_stmt(Cursor& c) {
    switch (c.type()) {
    case SyntaxType::ExprStmt: {
        auto expr = build_expr(c.expect_node());
        c.accept_token(TokenType::Semicolon);
        c.expect_end();

        auto stmt = make_node<AstExprStmt>();
        stmt->expr(std::move(expr));
        return stmt;
    }

    case SyntaxType::DeferStmt:
    case SyntaxType::AssertStmt:
    case SyntaxType::VarStmt:
    case SyntaxType::WhileStmt:
    case SyntaxType::ForStmt:
    case SyntaxType::ForEachStmt:
    default:
        err(c.type(), "syntax type is not supported in statement context");
    }
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_expr(Cursor& c) {
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
    case SyntaxType::IfExpr:
        return build_if(c);
    case SyntaxType::BlockExpr:
        return build_block(c);
    case SyntaxType::FuncExpr:
        return build_func(c);
    case SyntaxType::CallExpr:
        return build_call(c);
    case SyntaxType::ConstructExpr:
        return build_construct(c);

    case SyntaxType::RecordExpr:
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
        auto maybe_member = cursor_for(member_id);
        if (!maybe_member)
            return nullptr;

        auto& member_cursor = *maybe_member;
        auto name = member_cursor.expect_token({TokenType::Identifier, TokenType::TupleField});
        member_cursor.expect_end();

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
        auto maybe_node = cursor_for(c.expect_node());
        if (!maybe_node)
            continue;

        auto& string_cursor = *maybe_node;
        if (string_cursor.type() != SyntaxType::StringExpr)
            err(string_cursor.type(), "invalid item type in string group");
        gather_string_contents(items, string_cursor);
    }
    c.expect_end();

    auto string = make_node<AstStringExpr>();
    string->items(std::move(items));
    return string;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_if(Cursor& c) {
    c.expect_token(TokenType::KwIf);
    auto cond = build_cond(c.expect_node());
    auto then_branch = build_expr(c.expect_node());

    AstPtr<AstExpr> else_branch;
    if (c.accept_token(TokenType::KwElse)) {
        else_branch = build_expr(c.expect_node()).get();
    }
    c.expect_end();

    auto expr = make_node<AstIfExpr>();
    expr->cond(std::move(cond));
    expr->then_branch(std::move(then_branch));
    expr->else_branch(std::move(else_branch));
    return expr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_block(Cursor& c) {
    AstNodeList<AstStmt> stmts;
    c.expect_token(TokenType::LeftBrace);
    while (!c.at_end() && !c.at_token(TokenType::RightBrace)) {
        if (c.accept_token(TokenType::Semicolon))
            continue;

        stmts.append(build_stmt(c.expect_node()));
    }
    c.expect_token(TokenType::RightBrace);
    c.expect_end();

    auto node = make_node<AstBlockExpr>();
    node->stmts(std::move(stmts));
    return node;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_func(Cursor& c) {
    auto decl = build_func_decl(c.expect_node(SyntaxType::Func));
    c.expect_end();

    auto expr = make_node<AstFuncExpr>();
    expr->decl(std::move(decl));
    return expr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_call(Cursor& c) {
    auto func = build_expr(c.expect_node());
    auto arglist = build_args(c.expect_node());
    if (!arglist)
        return expr_error(c.id());

    auto& [access_type, args] = *arglist;

    auto call = make_node<AstCallExpr>(access_type);
    call->func(std::move(func));
    call->args(std::move(args));
    return call;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_construct(Cursor& c) {
    auto ident = c.expect_token(TokenType::Identifier);
    auto name = source(ident);
    if (name == "map") {
        auto items = gather_map_items(c);
        if (!items)
            return expr_error(c.id());

        auto map = make_node<AstMapLiteral>();
        map->items(std::move(*items));
        return map;
    }

    if (name == "set") {
        auto items = gather_set_items(c);
        if (!items)
            return expr_error(c.id());

        auto set = make_node<AstSetLiteral>();
        set->items(std::move(*items));
        return set;
    }

    diag_.reportf(Diagnostics::Error, ident.range(),
        "invalid constructor expressions (expected 'map' or 'set').");
    return expr_error(c.id());
}

AstPtr<AstExpr> AstBuilder::build_cond(SyntaxNodeId node_id) {
    auto maybe_cond = cursor_for(node_id);
    if (!maybe_cond)
        return nullptr;

    auto& cond_cursor = *maybe_cond;
    if (cond_cursor.type() != SyntaxType::Condition)
        err(cond_cursor.type(), "expected a condition");

    auto expr = build_expr(cond_cursor.expect_node());
    cond_cursor.expect_end();
    return std::move(expr).get();
}

AstPtr<AstFuncDecl> AstBuilder::build_func_decl(SyntaxNodeId node_id) {
    auto maybe_func = cursor_for(node_id);
    if (!maybe_func)
        return nullptr;

    auto& func_cursor = *maybe_func;
    if (func_cursor.type() != SyntaxType::Func)
        err(func_cursor.type(), "expected a function");

    AstNodeList<AstModifier> modifiers;
    if (auto modifiers_id = func_cursor.accept_node(SyntaxType::Modifiers)) {
        auto modifiers_cursor = cursor_for(*modifiers_id);
        if (modifiers_cursor)
            gather_modifiers(modifiers, *modifiers_cursor);
    }

    func_cursor.expect_token(TokenType::KwFunc);

    std::optional<std::string_view> name;
    if (auto maybe_name = func_cursor.accept_node(SyntaxType::Name)) {
        name = build_name(*maybe_name);
    }

    AstNodeList<AstParamDecl> params;
    auto param_list = func_cursor.expect_node(SyntaxType::ParamList);
    if (auto param_cursor = cursor_for(param_list))
        gather_params(params, *param_cursor);

    bool body_is_value = func_cursor.accept_token(TokenType::Equals).has_value();
    auto body = build_expr(func_cursor.expect_node());
    func_cursor.expect_end();

    auto func = make_node<AstFuncDecl>();
    if (name)
        func->name(strings_.insert(*name));
    func->body_is_value(body_is_value);
    func->modifiers(std::move(modifiers));
    func->params(std::move(params));
    func->body(std::move(body));
    return std::move(func).get();
}

std::optional<std::string_view> AstBuilder::build_name(SyntaxNodeId id) {
    auto maybe_name = cursor_for(id);
    if (!maybe_name)
        return {};

    auto& name_cursor = *maybe_name;
    if (name_cursor.type() != SyntaxType::Name)
        err(name_cursor.type(), "expected a name");

    auto ident = name_cursor.expect_token(TokenType::Identifier);
    name_cursor.expect_end();
    return source(ident);
}

std::optional<std::tuple<AccessType, AstNodeList<AstExpr>>>
AstBuilder::build_args(SyntaxNodeId args_id) {
    auto maybe_cursor = cursor_for(args_id);
    if (!maybe_cursor)
        return {};

    auto& args_cursor = *maybe_cursor;
    if (args_cursor.type() != SyntaxType::ArgList)
        err(args_cursor.type(), "expected an argument list");

    auto open_paren = args_cursor.expect_token({
        TokenType::LeftParen,
        TokenType::QuestionLeftParen,
    });

    AstNodeList<AstExpr> args;
    while (!args_cursor.at_end() && !args_cursor.at_token(TokenType::RightParen)) {
        args.append(build_expr(args_cursor.expect_node()));
        if (args_cursor.at_token(TokenType::RightParen))
            break;

        args_cursor.expect_token(TokenType::Comma);
    }
    args_cursor.expect_token(TokenType::RightParen);
    args_cursor.expect_end();

    auto access_type = open_paren.type() == TokenType::QuestionLeftParen ? AccessType::Optional
                                                                         : AccessType::Normal;
    return std::tuple(access_type, std::move(args));
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

        auto maybe_item = cursor_for(c.expect_node());
        if (!maybe_item)
            continue;

        auto& item_cursor = *maybe_item;
        switch (maybe_item->type()) {
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

void AstBuilder::gather_params(AstNodeList<AstParamDecl>& params, Cursor& c) {
    c.expect_token(TokenType::LeftParen);
    while (!c.at_end() && !c.at_token(TokenType::RightParen)) {
        auto param_name = c.expect_token(TokenType::Identifier);
        auto param = make_node<AstParamDecl>();
        param->name(strings_.insert(source(param_name)));
        params.append(std::move(param));

        if (c.at_token(TokenType::RightParen))
            break;

        c.expect_token(TokenType::Comma);
    }
    c.expect_token(TokenType::RightParen);
    c.expect_end();
}

void AstBuilder::gather_modifiers(AstNodeList<AstModifier>& modifiers, Cursor& c) {
    bool has_export = false;

    while (!c.at_end()) {
        auto modifier = c.expect_token({TokenType::KwExport});

        switch (modifier.type()) {
        case TokenType::KwExport:
            if (has_export)
                diag_.report(Diagnostics::Error, modifier.range(), "redundant export modifier");
            has_export = true;
            modifiers.append(make_node<AstExportModifier>());
            break;

        default:
            err(c.type(), "invalid modifier");
        }
    }
    c.expect_end();
}

std::optional<AstNodeList<AstMapItem>> AstBuilder::gather_map_items(Cursor& c) {
    c.expect_token(TokenType::LeftBrace);

    AstNodeList<AstMapItem> items;
    while (!c.at_end() && !c.at_token(TokenType::RightBrace)) {
        auto key = build_expr(c.expect_node());

        // Parser lets this through
        if (!c.accept_token(TokenType::Colon)) {
            diag_.reportf(Diagnostics::Error, c.next_range(),
                "expected {} after this key in map literal", to_description(TokenType::Colon));
            return {};
        }

        auto value = build_expr(c.expect_node());

        auto item = make_node<AstMapItem>();
        item->key(std::move(key));
        item->value(std::move(value));
        items.append(std::move(item));

        if (!c.at_token(TokenType::RightBrace))
            c.expect_token(TokenType::Comma);
    }

    c.expect_token(TokenType::RightBrace);
    c.expect_end();
    return items;
}

std::optional<AstNodeList<AstExpr>> AstBuilder::gather_set_items(Cursor& c) {
    c.expect_token(TokenType::LeftBrace);

    AstNodeList<AstExpr> items;
    while (!c.at_end() && !c.at_token(TokenType::RightBrace)) {
        items.append(build_expr(c.expect_node()));

        if (auto colon = c.accept_token(TokenType::Colon)) {
            diag_.reportf(Diagnostics::Error, colon->range(), "unexpected {} in set literal",
                to_description(TokenType::Colon));
            return {};
        }

        if (!c.at_token(TokenType::RightBrace))
            c.expect_token(TokenType::Comma);
    }

    c.expect_token(TokenType::RightBrace);
    c.expect_end();
    return items;
}

SyntaxNodeId AstBuilder::get_syntax_node() {
    // Visit the root node and emit errors.
    // The root node contains errors that could not be attached to any open syntax node during parsing.
    auto root_id = tree_.root_id();
    TIRO_DEBUG_ASSERT(root_id, "Syntax tree does not have a root.");

    auto maybe_root = cursor_for(root_id);
    if (!maybe_root)
        return {};

    auto child_id = maybe_root->expect_node();
    maybe_root->expect_end();
    return child_id;
}

NotNull<AstPtr<AstExpr>> AstBuilder::expr_error(SyntaxNodeId node_id) {
    // FIXME Build exprerror instance
    (void) node_id;
    TIRO_ERROR("Error expression");
}

NotNull<AstPtr<AstStmt>> AstBuilder::stmt_error(SyntaxNodeId node_id) {
    // FIXME Build stmterror instance
    (void) node_id;
    TIRO_ERROR("Error statement");
}

std::optional<Cursor> AstBuilder::cursor_for(SyntaxNodeId node_id) {
    auto node_data = tree_[node_id];
    if (node_data->type() == SyntaxType::Error || !node_data->errors().empty())
        return {};
    return Cursor(tree_, node_id, node_data);
}

void emit_errors(const SyntaxTree& tree, Diagnostics& diag) {
    auto emit = [&](const SourceRange& range, std::string message) {
        diag.report(Diagnostics::Error, range, std::move(message));
    };

    Fix emit_recursive = [&](auto& self, SyntaxNodeId node_id) -> void {
        if (!node_id)
            return;

        auto node_data = tree[node_id];
        for (const auto& error : node_data->errors())
            emit(node_data->range(), error);

        if (node_data->type() == SyntaxType::Error && node_data->errors().empty())
            emit(node_data->range(), "syntax error");

        for (const auto& child : node_data->children()) {
            if (child.type() == SyntaxChildType::NodeId)
                self(child.as_node_id());
        }
    };

    emit_recursive(tree.root_id());
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
