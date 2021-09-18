#include "compiler/ast_gen/build_ast.hpp"

#include "common/error.hpp"
#include "common/fix.hpp"
#include "compiler/ast/ast.hpp"
#include "compiler/ast_gen/node_reader.hpp"
#include "compiler/ast_gen/operators.hpp"
#include "compiler/ast_gen/typed_nodes.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/syntax/grammar/literals.hpp"
#include "compiler/syntax/syntax_tree.hpp"
#include "compiler/syntax/syntax_type.hpp"
#include "compiler/syntax/token_set.hpp"

#include <optional>
#include <string_view>

namespace tiro {

static void emit_errors(const SyntaxTree& tree, Diagnostics& diag);

namespace {

struct BuilderState {
public:
    explicit BuilderState(Diagnostics& diag, StringTable& strings)
        : diag_(diag)
        , strings_(strings) {}

    Diagnostics& diag() { return diag_; }

    StringTable& strings() { return strings_; }

    // Buffer for parse time string concatenation.
    std::string& buffer() { return buffer_; }

    // Generates a new unique ast node id.
    AstId next_node_id() {
        static_assert(std::is_same_v<u32, AstId::UnderlyingType>);

        u32 value = next_node_id_++;
        if (TIRO_UNLIKELY(value == AstId::invalid_value))
            TIRO_ERROR("too many ast nodes");
        return AstId(value);
    }

private:
    Diagnostics& diag_;
    StringTable& strings_;
    std::string buffer_;
    u32 next_node_id_ = 1;
};

/// Implements the syntax tree -> abstract syntax tree transformation.
class AstBuilder {
public:
    explicit AstBuilder(const SyntaxTree& tree, BuilderState& state)
        : tree_(tree)
        , state_(state) {}

    template<typename Node, typename Func>
    AstPtr<Node> build(Func&& fn);

    NotNull<AstPtr<AstFile>> build_file(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstStmt>> build_stmt(SyntaxNodeId node_id);
    NotNull<AstPtr<AstStmt>> build_item(SyntaxNodeId node_id);

private:
    // Expressions
    NotNull<AstPtr<AstExpr>> build_var_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_literal_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_group_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_return_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_field_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_tuple_field_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_index_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_binary_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_unary_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_array_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_tuple_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_record_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_set_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_map_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_string_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_string_group_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_if_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_block_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_func_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstExpr>> build_call_expr(SyntaxNodeId node_id);

    // Statements
    NotNull<AstPtr<AstStmt>> build_expr_stmt(SyntaxNodeId node_id);
    NotNull<AstPtr<AstStmt>> build_defer_stmt(SyntaxNodeId node_id);
    NotNull<AstPtr<AstStmt>> build_assert_stmt(SyntaxNodeId node_id);
    NotNull<AstPtr<AstStmt>> build_var_stmt(SyntaxNodeId node_id);
    NotNull<AstPtr<AstStmt>> build_while_stmt(SyntaxNodeId node_id);
    NotNull<AstPtr<AstStmt>> build_for_stmt(SyntaxNodeId node_id);
    NotNull<AstPtr<AstStmt>> build_for_each_stmt(SyntaxNodeId node_id);

    // Items
    NotNull<AstPtr<AstStmt>> build_var_item(SyntaxNodeId node_id);
    NotNull<AstPtr<AstStmt>> build_func_item(SyntaxNodeId node_id);
    NotNull<AstPtr<AstStmt>> build_import_item(SyntaxNodeId node_id);

    // Helpers
    NotNull<AstPtr<AstExpr>> build_cond(SyntaxNodeId node_id);
    std::optional<std::string_view> build_name(SyntaxNodeId node_id);
    AstPtr<AstFuncDecl> build_func_decl(SyntaxNodeId node_id);
    AstPtr<AstVarDecl> build_var_decl(SyntaxNodeId node_id);
    AstPtr<AstBinding> build_binding(SyntaxNodeId node_id, bool is_const);
    AstPtr<AstBindingSpec> build_spec(SyntaxNodeId node_id);

    std::optional<std::tuple<AccessType, AstNodeList<AstExpr>>> build_args(SyntaxNodeId node_id);

    void gather_string_contents(AstNodeList<AstExpr>& items, SyntaxNodeId node_id);
    void gather_params(AstNodeList<AstParamDecl>& params, SyntaxNodeId node_id);
    void gather_modifiers(AstNodeList<AstModifier>& modifiers, SyntaxNodeId node_id);

private:
    Diagnostics& diag() const { return state_.diag(); }
    StringTable& strings() const { return state_.strings(); }

    // Returns the topmost syntax node (direct child of the root) or an invalid id if
    // the root contains errors.
    SyntaxNodeId get_syntax_node();

    NotNull<AstPtr<AstExpr>> error_expr(SyntaxNodeId node_id);
    NotNull<AstPtr<AstStmt>> error_stmt(SyntaxNodeId node_id);

    std::string_view source(const Token& token) const {
        return substring(tree_.source(), token.range());
    }

    auto diag_sink(const SourceRange& range) const {
        return [&diag = this->diag(), range](std::string_view error_message) {
            diag.report(Diagnostics::Error, range, std::string(error_message));
        };
    }

    // Returns the type of the given syntax node. Returns an empty optional
    // if the node contains errors, in which case it should not be visited.
    std::optional<SyntaxType> type(SyntaxNodeId node_id) {
        const auto& node_data = tree_[node_id];
        if (node_data.type() == SyntaxType::Error || node_data.has_error())
            return {};

        return node_data.type();
    }

    SourceRange range(SyntaxNodeId node_id) { return tree_[node_id].range(); }

    template<SyntaxType st>
    std::optional<typed_syntax::NodeType<st>> read(SyntaxNodeId node_id) {
        TIRO_DEBUG_ASSERT(type(node_id).has_value(), "nodes with errors should not be read");
        return typed_syntax::NodeReader(tree_).read<st>(node_id);
    }

    template<SyntaxType expected>
    std::optional<typed_syntax::NodeType<expected>> read_checked(SyntaxNodeId node_id) {
        auto node_type = type(node_id);
        if (!node_type)
            return {}; // Error node, not a fatal error

        if (*node_type != expected)
            unexpected(node_id, fmt::format("expected {}", expected));

        return read<expected>(node_id);
    }

    [[noreturn]] void unexpected(SyntaxNodeId node_id, std::string_view message) {
        auto node_type = tree_[node_id].type();
        TIRO_ERROR(
            "In node of type '{}': {}. This is either a bug in the parser or in "
            "the ast construction algorithm.",
            node_type, message);
    }

    template<typename T, typename... Args>
    NotNull<AstPtr<T>> make_node(SyntaxNodeId syntax_id, Args&&... args) {
        const auto& syntax_data = tree_[syntax_id];
        return make_node<T>(syntax_data.range(), std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    NotNull<AstPtr<T>> make_node(const SourceRange& range, Args&&... args) {
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        node->id(state_.next_node_id());
        node->range(range);
        return TIRO_NN(std::move(node));
    }

private:
    const SyntaxTree& tree_;
    BuilderState& state_;
};

} // namespace

static AstPtr<AstFile> build_file_ast(const SyntaxTree& file_tree, BuilderState& state) {
    AstBuilder builder(file_tree, state);
    return builder.build<AstFile>([&](auto node_id) { return builder.build_file(node_id); });
}

static AstPtr<AstStmt> build_item_ast(const SyntaxTree& item_tree, BuilderState& state) {
    AstBuilder builder(item_tree, state);
    return builder.build<AstStmt>([&](auto node_id) { return builder.build_item(node_id); });
}

static AstPtr<AstStmt> build_stmt_ast(const SyntaxTree& stmt_tree, BuilderState& state) {
    AstBuilder builder(stmt_tree, state);
    return builder.build<AstStmt>([&](auto node_id) { return builder.build_stmt(node_id); });
}

static AstPtr<AstExpr> build_expr_ast(const SyntaxTree& expr_tree, BuilderState& state) {
    AstBuilder builder(expr_tree, state);
    return builder.build<AstExpr>([&](auto node_id) { return builder.build_expr(node_id); });
}

AstPtr<AstModule>
build_module_ast(Span<const SyntaxTree> files, StringTable& strings, Diagnostics& diag) {
    BuilderState state(diag, strings);
    AstPtr<AstModule> mod = std::make_unique<AstModule>();
    mod->id(state.next_node_id());

    for (auto& file_tree : files) {
        auto file = build_file_ast(file_tree, state);
        mod->files().append(std::move(file));
    }
    return mod;
}

AstPtr<AstFile>
build_file_ast(const SyntaxTree& file_tree, StringTable& strings, Diagnostics& diag) {
    BuilderState state(diag, strings);
    return build_file_ast(file_tree, state);
}

AstPtr<AstStmt>
build_item_ast(const SyntaxTree& item_tree, StringTable& strings, Diagnostics& diag) {
    BuilderState state(diag, strings);
    return build_item_ast(item_tree, state);
}

AstPtr<AstStmt>
build_stmt_ast(const SyntaxTree& stmt_tree, StringTable& strings, Diagnostics& diag) {
    BuilderState state(diag, strings);
    return build_stmt_ast(stmt_tree, state);
}

AstPtr<AstExpr>
build_expr_ast(const SyntaxTree& expr_tree, StringTable& strings, Diagnostics& diag) {
    BuilderState state(diag, strings);
    return build_expr_ast(expr_tree, state);
}

template<typename Node, typename Func>
AstPtr<Node> AstBuilder::build(Func&& fn) {
    emit_errors(tree_, diag());

    SyntaxNodeId node_id = get_syntax_node();
    if (node_id) {
        return fn(node_id);
    }
    return nullptr;
}

NotNull<AstPtr<AstFile>> AstBuilder::build_file(SyntaxNodeId node_id) {
    auto node = read_checked<SyntaxType::File>(node_id);
    if (!node)
        unexpected(node_id, "expected a file");

    AstNodeList<AstStmt> items;
    for (const auto& item_id : node->items()) {
        items.append(build_item(item_id));
    }

    auto file = make_node<AstFile>(node_id);
    file->items(std::move(items));
    return file;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_expr(SyntaxNodeId node_id) {
    auto node_type = type(node_id);
    if (!node_type)
        return error_expr(node_id);

    switch (*node_type) {
    case SyntaxType::VarExpr:
        return build_var_expr(node_id);
    case SyntaxType::Literal:
        return build_literal_expr(node_id);
    case SyntaxType::GroupedExpr:
        return build_group_expr(node_id);
    case SyntaxType::ContinueExpr:
        return make_node<AstContinueExpr>(node_id);
    case SyntaxType::BreakExpr:
        return make_node<AstBreakExpr>(node_id);
    case SyntaxType::FieldExpr:
        return build_field_expr(node_id);
    case SyntaxType::TupleFieldExpr:
        return build_tuple_field_expr(node_id);
    case SyntaxType::IndexExpr:
        return build_index_expr(node_id);
    case SyntaxType::ReturnExpr:
        return build_return_expr(node_id);
    case SyntaxType::BinaryExpr:
        return build_binary_expr(node_id);
    case SyntaxType::UnaryExpr:
        return build_unary_expr(node_id);
    case SyntaxType::ArrayExpr:
        return build_array_expr(node_id);
    case SyntaxType::TupleExpr:
        return build_tuple_expr(node_id);
    case SyntaxType::RecordExpr:
        return build_record_expr(node_id);
    case SyntaxType::SetExpr:
        return build_set_expr(node_id);
    case SyntaxType::MapExpr:
        return build_map_expr(node_id);
    case SyntaxType::StringExpr:
        return build_string_expr(node_id);
    case SyntaxType::StringGroup:
        return build_string_group_expr(node_id);
    case SyntaxType::IfExpr:
        return build_if_expr(node_id);
    case SyntaxType::BlockExpr:
        return build_block_expr(node_id);
    case SyntaxType::FuncExpr:
        return build_func_expr(node_id);
    case SyntaxType::CallExpr:
        return build_call_expr(node_id);
    default:
        unexpected(node_id, "syntax type is not supported in expression context");
    }
}

NotNull<AstPtr<AstStmt>> AstBuilder::build_stmt(SyntaxNodeId node_id) {
    auto node_type = type(node_id);
    if (!node_type)
        return error_stmt(node_id);

    switch (*node_type) {
    case SyntaxType::ExprStmt:
        return build_expr_stmt(node_id);
    case SyntaxType::DeferStmt:
        return build_defer_stmt(node_id);
    case SyntaxType::AssertStmt:
        return build_assert_stmt(node_id);
    case SyntaxType::VarStmt:
        return build_var_stmt(node_id);
    case SyntaxType::WhileStmt:
        return build_while_stmt(node_id);
    case SyntaxType::ForStmt:
        return build_for_stmt(node_id);
    case SyntaxType::ForEachStmt:
        return build_for_each_stmt(node_id);
    default:
        unexpected(node_id, "syntax type is not supported in statement context");
    }
}

NotNull<AstPtr<AstStmt>> AstBuilder::build_item(SyntaxNodeId node_id) {
    auto node_type = type(node_id);
    if (!node_type)
        return error_stmt(node_id);

    switch (*node_type) {
    case SyntaxType::FuncItem:
        return build_func_item(node_id);
    case SyntaxType::VarItem:
        return build_var_item(node_id);
    case SyntaxType::ImportItem:
        return build_import_item(node_id);
    default:
        unexpected(node_id, "syntax type is not supported in item context");
    }
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_var_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::VarExpr>(node_id);
    if (!node)
        return error_expr(node_id);
    return make_node<AstVarExpr>(node_id, strings().insert(source(node->identifier)));
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_literal_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::Literal>(node_id);
    if (!node)
        return error_expr(node_id);

    auto& token = node->value;
    switch (token.type()) {
    case TokenType::KwTrue:
        return make_node<AstBooleanLiteral>(node_id, true);
    case TokenType::KwFalse:
        return make_node<AstBooleanLiteral>(node_id, false);
    case TokenType::KwNull:
        return make_node<AstNullLiteral>(node_id);
    case TokenType::Symbol: {
        auto name = parse_symbol_name(source(token), diag_sink(token.range()));
        if (!name)
            return error_expr(node_id);
        return make_node<AstSymbolLiteral>(node_id, strings().insert(*name));
    }
    case TokenType::Integer: {
        auto value = parse_integer_value(source(token), diag_sink(token.range()));
        if (!value)
            return error_expr(node_id);
        return make_node<AstIntegerLiteral>(node_id, *value);
    }
    case TokenType::Float: {
        auto value = parse_float_value(source(token), diag_sink(token.range()));
        if (!value)
            return error_expr(node_id);
        return make_node<AstFloatLiteral>(node_id, *value);
    }
    default:
        diag().reportf(Diagnostics::Error, token.range(), "unexpected {} in literal expression",
            to_description(token.type()));
        return error_expr(node_id);
    }
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_group_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::GroupedExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    return build_expr(node->expr);
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_return_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::ReturnExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    AstPtr<AstExpr> value;
    if (node->value) {
        value = build_expr(*node->value).get();
    }

    auto expr = make_node<AstReturnExpr>(node_id);
    expr->value(std::move(value));
    return expr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_field_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::FieldExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    auto instance = build_expr(node->instance);
    auto access = node->access.type() == TokenType::QuestionDot ? AccessType::Optional
                                                                : AccessType::Normal;
    auto name = strings().insert(source(node->field));

    auto expr = make_node<AstFieldExpr>(node_id, access, name);
    expr->instance(std::move(instance));
    return expr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_tuple_field_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::TupleFieldExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    auto instance = build_expr(node->instance);
    auto access = node->access.type() == TokenType::QuestionDot ? AccessType::Optional
                                                                : AccessType::Normal;
    auto index = parse_tuple_field(source(node->field), diag_sink(node->field.range()));
    if (!index)
        return error_expr(node_id);

    auto expr = make_node<AstTupleFieldExpr>(node_id, access, *index);
    expr->instance(std::move(instance));
    return expr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_index_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::IndexExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    auto instance = build_expr(node->instance);
    auto access = node->bracket.type() == TokenType::QuestionLeftBracket ? AccessType::Optional
                                                                         : AccessType::Normal;
    auto element = build_expr(node->index);

    auto expr = make_node<AstElementExpr>(node_id, access);
    expr->instance(std::move(instance));
    expr->element(std::move(element));
    return expr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_binary_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::BinaryExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    auto lhs = build_expr(node->lhs);
    auto op = to_binary_operator(node->op.type());
    if (!op) {
        diag().reportf(Diagnostics::Error, node->op.range(), "unexpected binary operator {}",
            to_description(node->op.type()));
        return error_expr(node_id);
    }
    auto rhs = build_expr(node->rhs);

    auto expr = make_node<AstBinaryExpr>(node_id, *op);
    expr->left(std::move(lhs));
    expr->right(std::move(rhs));
    return expr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_unary_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::UnaryExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    auto op = to_unary_operator(node->op.type());
    if (!op) {
        diag().reportf(Diagnostics::Error, node->op.range(), "unexpected unary operator {}",
            to_description(node->op.type()));
        return error_expr(node_id);
    }
    auto inner = build_expr(node->expr);

    auto expr = make_node<AstUnaryExpr>(node_id, *op);
    expr->inner(std::move(inner));
    return expr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_array_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::ArrayExpr>(node_id);

    AstNodeList<AstExpr> items;
    for (const auto& item : node->items())
        items.append(build_expr(item));

    auto array = make_node<AstArrayLiteral>(node_id);
    array->items(std::move(items));
    return array;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_tuple_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::TupleExpr>(node_id);

    AstNodeList<AstExpr> items;
    for (const auto& item : node->items())
        items.append(build_expr(item));

    auto tuple = make_node<AstTupleLiteral>(node_id);
    tuple->items(std::move(items));
    return tuple;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_record_expr(SyntaxNodeId node_id) {
    auto build_item = [&](SyntaxNodeId item_id) -> AstPtr<AstRecordItem> {
        auto node = read_checked<SyntaxType::RecordItem>(item_id);
        if (!node)
            return nullptr;

        auto name = build_name(node->name);
        if (!name)
            return nullptr;

        auto key = make_node<AstIdentifier>(range(node->name), strings().insert(*name));
        auto value = build_expr(node->value);

        auto item = make_node<AstRecordItem>(item_id);
        item->key(std::move(key));
        item->value(std::move(value));
        return std::move(item).get();
    };

    auto node = read<SyntaxType::RecordExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    AstNodeList<AstRecordItem> items;
    for (const auto& syntax_item : node->items()) {
        auto item = build_item(syntax_item);
        if (!item)
            continue;
        items.append(std::move(item));
    }

    auto record = make_node<AstRecordLiteral>(node_id);
    record->items(std::move(items));
    return record;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_set_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::SetExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    AstNodeList<AstExpr> items;
    for (const auto& item : node->items())
        items.append(build_expr(item));

    auto set = make_node<AstSetLiteral>(node_id);
    set->items(std::move(items));
    return set;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_map_expr(SyntaxNodeId node_id) {
    auto build_item = [&](SyntaxNodeId item_id) -> AstPtr<AstMapItem> {
        auto node = read_checked<SyntaxType::MapItem>(item_id);
        if (!node)
            return nullptr;

        auto key = build_expr(node->key);
        auto value = build_expr(node->value);

        auto item = make_node<AstMapItem>(item_id);
        item->key(std::move(key));
        item->value(std::move(value));
        return std::move(item).get();
    };

    auto node = read<SyntaxType::MapExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    AstNodeList<AstMapItem> items;
    for (const auto& syntax_item : node->items()) {
        auto item = build_item(syntax_item);
        if (!item)
            continue;
        items.append(std::move(item));
    }

    auto map = make_node<AstMapLiteral>(node_id);
    map->items(std::move(items));
    return map;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_string_expr(SyntaxNodeId node_id) {
    AstNodeList<AstExpr> items;
    gather_string_contents(items, node_id);

    auto string = make_node<AstStringExpr>(node_id);
    string->items(std::move(items));
    return string;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_string_group_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::StringGroup>(node_id);
    if (!node)
        return error_expr(node_id);

    AstNodeList<AstExpr> items;
    for (const auto& string_item : node->items())
        gather_string_contents(items, string_item);

    auto string = make_node<AstStringExpr>(node_id);
    string->items(std::move(items));
    return string;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_if_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::IfExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    auto cond = build_cond(node->cond);
    auto then_branch = build_expr(node->then_branch);

    AstPtr<AstExpr> else_branch;
    if (node->else_branch)
        else_branch = build_expr(*node->else_branch).get();

    auto expr = make_node<AstIfExpr>(node_id);
    expr->cond(std::move(cond));
    expr->then_branch(std::move(then_branch));
    expr->else_branch(std::move(else_branch));
    return expr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_block_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::BlockExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    AstNodeList<AstStmt> stmts;
    for (const auto& item : node->items())
        stmts.append(build_stmt(item));

    auto expr = make_node<AstBlockExpr>(node_id);
    expr->stmts(std::move(stmts));
    return expr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_func_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::FuncExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    auto decl = build_func_decl(node->func);
    auto expr = make_node<AstFuncExpr>(node_id);
    expr->decl(std::move(decl));
    return expr;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_call_expr(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::CallExpr>(node_id);
    if (!node)
        return error_expr(node_id);

    auto func = build_expr(node->func);
    auto arglist = build_args(node->args);
    if (!arglist)
        return error_expr(node_id);

    auto& [access_type, args] = *arglist;
    auto call = make_node<AstCallExpr>(node_id, access_type);
    call->func(std::move(func));
    call->args(std::move(args));
    return call;
}

NotNull<AstPtr<AstStmt>> AstBuilder::build_expr_stmt(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::ExprStmt>(node_id);
    if (!node)
        return error_stmt(node_id);

    auto expr = build_expr(node->expr);
    auto stmt = make_node<AstExprStmt>(node_id);
    stmt->expr(std::move(expr));
    return stmt;
}

NotNull<AstPtr<AstStmt>> AstBuilder::build_defer_stmt(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::DeferStmt>(node_id);
    if (!node)
        return error_stmt(node_id);

    auto expr = build_expr(node->expr);
    auto stmt = make_node<AstDeferStmt>(node_id);
    stmt->expr(std::move(expr));
    return stmt;
}

NotNull<AstPtr<AstStmt>> AstBuilder::build_assert_stmt(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::AssertStmt>(node_id);
    if (!node)
        return error_stmt(node_id);

    auto arglist = build_args(node->args);
    if (!arglist)
        return error_stmt(node_id);

    auto& [access_type, args] = *arglist;
    if (access_type != AccessType::Normal) {
        diag().report(
            Diagnostics::Error, range(node_id), "assert only supports normal call syntax");
        return error_stmt(node_id);
    }
    if (!(args.size() == 1 || args.size() == 2)) {
        diag().report(Diagnostics::Error, range(node_id), "assert requires 1 or 2 arguments");
        return error_stmt(node_id);
    }

    auto stmt = make_node<AstAssertStmt>(node_id);
    stmt->cond(args.take(0));
    if (args.size() > 1)
        stmt->message(args.take(1));
    return stmt;
}

NotNull<AstPtr<AstStmt>> AstBuilder::build_var_stmt(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::VarStmt>(node_id);
    if (!node)
        return error_stmt(node_id);

    auto var = build_var_decl(node->var);
    if (!var)
        return error_stmt(node_id);

    auto stmt = make_node<AstDeclStmt>(node_id);
    stmt->decl(std::move(var));
    return stmt;
}

NotNull<AstPtr<AstStmt>> AstBuilder::build_while_stmt(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::WhileStmt>(node_id);
    if (!node)
        return error_stmt(node_id);

    auto cond = build_cond(node->cond);
    auto body = build_expr(node->body);

    auto stmt = make_node<AstWhileStmt>(node_id);
    stmt->cond(std::move(cond));
    stmt->body(std::move(body));
    return stmt;
}

NotNull<AstPtr<AstStmt>> AstBuilder::build_for_stmt(SyntaxNodeId node_id) {
    auto build_header = [this](SyntaxNodeId header_id)
        -> std::optional<std::tuple<AstPtr<AstVarDecl>, AstPtr<AstExpr>, AstPtr<AstExpr>>> {
        auto node = read_checked<SyntaxType::ForStmtHeader>(header_id);
        if (!node)
            return {};

        AstPtr<AstVarDecl> decl;
        if (node->decl) {
            decl = build_var_decl(*node->decl);
            if (!decl)
                return {};
        }

        AstPtr<AstExpr> cond;
        if (node->cond)
            cond = build_cond(*node->cond).get();

        AstPtr<AstExpr> step;
        if (node->step)
            step = build_expr(*node->step).get();

        return std::tuple(std::move(decl), std::move(cond), std::move(step));
    };

    auto node = read<SyntaxType::ForStmt>(node_id);
    if (!node)
        return error_stmt(node_id);

    auto header = build_header(node->header);
    if (!header)
        return error_stmt(node_id);
    auto& [decl, cond, step] = *header;

    auto body = build_expr(node->body);

    auto stmt = make_node<AstForStmt>(node_id);
    stmt->decl(std::move(decl));
    stmt->cond(std::move(cond));
    stmt->step(std::move(step));
    stmt->body(std::move(body));
    return stmt;
}

NotNull<AstPtr<AstStmt>> AstBuilder::build_for_each_stmt(SyntaxNodeId node_id) {
    auto build_header = [this](SyntaxNodeId header_id)
        -> std::optional<std::tuple<AstPtr<AstBindingSpec>, AstPtr<AstExpr>>> {
        auto node = read_checked<SyntaxType::ForEachStmtHeader>(header_id);
        if (!node)
            return {};

        auto spec = build_spec(node->spec);
        if (!spec)
            return {};

        auto expr = build_expr(node->expr);
        return std::tuple(std::move(spec), std::move(expr));
    };

    auto node = read<SyntaxType::ForEachStmt>(node_id);
    if (!node)
        return error_stmt(node_id);

    auto header = build_header(node->header);
    if (!header)
        return error_stmt(node_id);
    auto& [spec, expr] = *header;

    auto body = build_expr(node->body);

    auto stmt = make_node<AstForEachStmt>(node_id);
    stmt->spec(std::move(spec));
    stmt->expr(std::move(expr));
    stmt->body(std::move(body));
    return stmt;
}

NotNull<AstPtr<AstStmt>> AstBuilder::build_func_item(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::FuncItem>(node_id);
    if (!node)
        return error_stmt(node_id);

    auto func = build_func_decl(node->func);
    if (!func)
        return error_stmt(node_id);

    auto stmt = make_node<AstDeclStmt>(node_id);
    stmt->decl(std::move(func));
    return stmt;
}

NotNull<AstPtr<AstStmt>> AstBuilder::build_var_item(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::VarItem>(node_id);
    if (!node)
        return error_stmt(node_id);

    auto var = build_var_decl(node->var);
    if (!var)
        return error_stmt(node_id);

    auto stmt = make_node<AstDeclStmt>(node_id);
    stmt->decl(std::move(var));
    return stmt;
}

NotNull<AstPtr<AstStmt>> AstBuilder::build_import_item(SyntaxNodeId node_id) {
    auto node = read<SyntaxType::ImportItem>(node_id);
    if (!node)
        return error_stmt(node_id);

    std::vector<InternedString> path;
    for (const auto& ident : node->path()) {
        TIRO_DEBUG_ASSERT(ident.type() == TokenType::Identifier, "expected identifier");
        path.push_back(strings().insert(source(ident)));
    }

    if (path.empty())
        unexpected(node_id, "empty import path");

    auto decl = make_node<AstImportDecl>(node_id);
    decl->name(path.back());
    decl->path(std::move(path));

    auto stmt = make_node<AstDeclStmt>(node_id);
    stmt->decl(std::move(decl));
    return stmt;
}

NotNull<AstPtr<AstExpr>> AstBuilder::build_cond(SyntaxNodeId node_id) {
    auto node = read_checked<SyntaxType::Condition>(node_id);
    if (!node)
        return error_expr(node_id);

    return build_expr(node->expr);
}

std::optional<std::string_view> AstBuilder::build_name(SyntaxNodeId node_id) {
    auto node = read_checked<SyntaxType::Name>(node_id);
    if (!node)
        return {};

    const auto& ident = node->value;
    TIRO_DEBUG_ASSERT(ident.type() == TokenType::Identifier, "expected identifier");
    return source(ident);
}

AstPtr<AstFuncDecl> AstBuilder::build_func_decl(SyntaxNodeId node_id) {
    auto node = read_checked<SyntaxType::Func>(node_id);
    if (!node)
        return nullptr;

    AstNodeList<AstModifier> modifiers;
    if (node->modifiers)
        gather_modifiers(modifiers, *node->modifiers);

    std::optional<std::string_view> name;
    if (node->name)
        name = build_name(*node->name);

    AstNodeList<AstParamDecl> params;
    gather_params(params, node->params);

    auto body = build_expr(node->body);

    auto func = make_node<AstFuncDecl>(node_id);
    if (name)
        func->name(strings().insert(*name));
    func->body_is_value(node->body_is_value);
    func->modifiers(std::move(modifiers));
    func->params(std::move(params));
    func->body(std::move(body));
    return std::move(func).get();
}

AstPtr<AstVarDecl> AstBuilder::build_var_decl(SyntaxNodeId node_id) {
    auto node = read_checked<SyntaxType::Var>(node_id);
    if (!node)
        return nullptr;

    AstNodeList<AstModifier> modifiers;
    if (auto modifiers_id = node->modifiers)
        gather_modifiers(modifiers, *modifiers_id);

    const auto& keyword = node->decl;
    TIRO_DEBUG_ASSERT(keyword.type() == TokenType::KwConst || keyword.type() == TokenType::KwVar,
        "unexpected var declaration keyword");
    bool is_const = keyword.type() == TokenType::KwConst;

    AstNodeList<AstBinding> bindings;
    for (const auto& binding_id : node->bindings()) {
        auto binding = build_binding(binding_id, is_const);
        if (binding)
            bindings.append(std::move(binding));
    }

    auto decl = make_node<AstVarDecl>(node_id);
    decl->modifiers(std::move(modifiers));
    decl->bindings(std::move(bindings));
    return std::move(decl).get();
}

AstPtr<AstBinding> AstBuilder::build_binding(SyntaxNodeId node_id, bool is_const) {
    auto node = read_checked<SyntaxType::Binding>(node_id);
    if (!node)
        return nullptr;

    auto spec = build_spec(node->spec);
    if (!spec)
        return nullptr;

    AstPtr<AstExpr> init;
    if (node->init)
        init = build_expr(*node->init).get();

    auto binding = make_node<AstBinding>(node_id, is_const);
    binding->spec(std::move(spec));
    binding->init(std::move(init));
    return std::move(binding).get();
}

AstPtr<AstBindingSpec> AstBuilder::build_spec(SyntaxNodeId node_id) {
    auto node_type = type(node_id);
    if (!node_type)
        return {};

    auto make_name = [&](const Token& ident) {
        auto name_interned = strings().insert(source(ident));
        return make_node<AstIdentifier>(ident.range(), name_interned);
    };

    switch (*node_type) {
    case SyntaxType::BindingName: {
        auto node = read<SyntaxType::BindingName>(node_id);
        if (!node)
            return {};

        auto spec = make_node<AstVarBindingSpec>(node_id);
        spec->name(make_name(node->name));
        return std::move(spec).get();
    }

    case SyntaxType::BindingTuple: {
        auto node = read<SyntaxType::BindingTuple>(node_id);
        if (!node)
            return {};

        AstNodeList<AstIdentifier> names;
        for (const auto& name_token : node->names()) {
            TIRO_DEBUG_ASSERT(name_token.type() == TokenType::Identifier, "expected identifier");
            names.append(make_name(name_token));
        }

        auto spec = make_node<AstTupleBindingSpec>(node_id);
        spec->names(std::move(names));
        return std::move(spec).get();
    }
    default:
        unexpected(node_id, "syntax type not allowed in binding context");
    }
}

std::optional<std::tuple<AccessType, AstNodeList<AstExpr>>>
AstBuilder::build_args(SyntaxNodeId node_id) {
    auto node = read_checked<SyntaxType::ArgList>(node_id);
    if (!node)
        return {};

    const auto& paren = node->paren.type();
    TIRO_DEBUG_ASSERT(paren == TokenType::LeftParen || paren == TokenType::QuestionLeftParen,
        "unexpected opening parens");

    AstNodeList<AstExpr> args;
    for (const auto& arg_item : node->items())
        args.append(build_expr(arg_item));

    auto access_type = paren == TokenType::QuestionLeftParen ? AccessType::Optional
                                                             : AccessType::Normal;
    return std::tuple(access_type, std::move(args));
}

void AstBuilder::gather_string_contents(AstNodeList<AstExpr>& items, SyntaxNodeId node_id) {
    struct ItemVisitor {
        AstBuilder& self;
        AstNodeList<AstExpr>& items;

        void visit_token(const Token& content) {
            TIRO_DEBUG_ASSERT(
                content.type() == TokenType::StringContent, "expected string content");

            auto& buffer = self.state_.buffer();
            buffer.clear();
            parse_string_literal(self.source(content), buffer, self.diag_sink(content.range()));
            items.append(
                self.make_node<AstStringLiteral>(content.range(), self.strings().insert(buffer)));
        }

        void visit_node_id(SyntaxNodeId child_id) {
            auto child_type = self.type(child_id);
            if (!child_type)
                return;

            switch (*child_type) {
            case SyntaxType::StringFormatItem: {
                auto child_node = self.read<SyntaxType::StringFormatItem>(child_id);
                if (!child_node)
                    return;

                items.append(self.build_expr(child_node->expr));
                break;
            }
            case SyntaxType::StringFormatBlock: {
                auto child_node = self.read<SyntaxType::StringFormatBlock>(child_id);
                if (!child_node)
                    return;

                items.append(self.build_expr(child_node->expr));
                break;
            }
            default:
                self.unexpected(child_id, "invalid string item type");
            }
        }
    };

    auto node = read_checked<SyntaxType::StringExpr>(node_id);
    if (!node)
        return;

    ItemVisitor visitor{*this, items};
    for (const auto& item : node->items())
        item.visit(visitor);
}

void AstBuilder::gather_params(AstNodeList<AstParamDecl>& params, SyntaxNodeId node_id) {
    auto node = read_checked<SyntaxType::ParamList>(node_id);
    if (!node)
        return;

    for (const auto& param_name : node->names()) {
        TIRO_DEBUG_ASSERT(param_name.type() == TokenType::Identifier, "expected identifier");
        auto param = make_node<AstParamDecl>(param_name.range());
        param->name(strings().insert(source(param_name)));
        params.append(std::move(param));
    }
}

void AstBuilder::gather_modifiers(AstNodeList<AstModifier>& modifiers, SyntaxNodeId node_id) {
    auto node = read_checked<SyntaxType::Modifiers>(node_id);
    if (!node)
        return;

    bool has_export = false;
    for (const auto& modifier : node->items()) {
        switch (modifier.type()) {
        case TokenType::KwExport:
            if (has_export)
                diag().report(Diagnostics::Error, modifier.range(), "redundant export modifier");
            has_export = true;
            modifiers.append(make_node<AstExportModifier>(modifier.range()));
            break;

        default:
            unexpected(node_id, "invalid modifier");
        }
    }
}

SyntaxNodeId AstBuilder::get_syntax_node() {
    // Visit the root node and emit errors.
    // The root node contains errors that could not be attached to any open syntax node during parsing.
    auto root_id = tree_.root_id();
    TIRO_DEBUG_ASSERT(root_id, "Syntax tree does not have a root.");

    auto node = read_checked<SyntaxType::Root>(root_id);
    if (!node)
        return {};

    return node->item;
}

NotNull<AstPtr<AstExpr>> AstBuilder::error_expr(SyntaxNodeId node_id) {
    return make_node<AstErrorExpr>(node_id);
}

NotNull<AstPtr<AstStmt>> AstBuilder::error_stmt(SyntaxNodeId node_id) {
    return make_node<AstErrorStmt>(node_id);
}

void emit_errors(const SyntaxTree& tree, Diagnostics& diag) {
    for (const auto& error : tree.errors()) {
        diag.report(Diagnostics::Error, error.range(), error.message());
    }
}

} // namespace tiro
