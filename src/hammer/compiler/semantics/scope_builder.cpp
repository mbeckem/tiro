#include "hammer/compiler/semantics/scope_builder.hpp"

#include "hammer/compiler/diagnostics.hpp"

namespace hammer::compiler {

ScopeBuilder::ScopeBuilder(SymbolTable& symbols, StringTable& strings,
    Diagnostics& diag, ScopePtr global_scope)
    : symbols_(symbols)
    , strings_(strings)
    , diag_(diag)
    , global_scope_(global_scope)
    , current_scope_(nullptr) {}

ScopeBuilder::~ScopeBuilder() {}

void ScopeBuilder::dispatch(const NodePtr<>& node) {
    if (node && !node->has_error()) {
        // Perform type specific actions.
        visit(node, *this);
    }
}

void ScopeBuilder::visit_root(const NodePtr<Root>& root) {
    root->root_scope(global_scope_);

    auto exit_scope = enter_scope(global_scope_);
    dispatch_children(root);
}

void ScopeBuilder::visit_file(const NodePtr<File>& file) {
    const auto scope = symbols_.create_scope(
        ScopeType::File, current_scope_, current_func_);
    file->file_scope(scope);

    auto exit_scope = enter_scope(scope);
    dispatch_children(file);
}

void ScopeBuilder::visit_func_decl(const NodePtr<FuncDecl>& func) {
    // TODO: Decls should always have a valid name (?). Dont make anon functions decls.
    if (func->name())
        add_decl(func);

    auto exit_func = enter_func(func);

    const auto param_scope = symbols_.create_scope(
        ScopeType::Parameters, current_scope_, current_func_);
    func->param_scope(param_scope);

    const auto body_scope = symbols_.create_scope(
        ScopeType::FunctionBody, param_scope, current_func_);
    func->body_scope(body_scope);

    auto exit_param_scope = enter_scope(param_scope);
    dispatch(func->params());

    auto exit_body_scope = enter_scope(body_scope);
    dispatch(func->body());
}

void ScopeBuilder::visit_decl(const NodePtr<Decl>& decl) {
    // TODO: Decls should always have a valid name.
    if (decl->name())
        add_decl(decl);

    dispatch_children(decl);
}

void ScopeBuilder::visit_for_stmt(const NodePtr<ForStmt>& stmt) {
    const auto decl_scope = symbols_.create_scope(
        ScopeType::ForStmtDecls, current_scope_, current_func_);
    stmt->decl_scope(decl_scope);

    const auto body_scope = symbols_.create_scope(
        ScopeType::LoopBody, decl_scope, current_func_);
    stmt->body_scope(body_scope);

    auto exit_decl_scope = enter_scope(decl_scope);
    dispatch(stmt->decl());
    dispatch(stmt->condition());
    dispatch(stmt->step());

    auto exit_body_scope = enter_scope(body_scope);
    dispatch(stmt->body());
}

void ScopeBuilder::visit_while_stmt(const NodePtr<WhileStmt>& stmt) {
    const auto body_scope = symbols_.create_scope(
        ScopeType::LoopBody, current_scope_, current_func_);
    stmt->body_scope(body_scope);

    dispatch(stmt->condition());

    auto exit_body_scope = enter_scope(body_scope);
    dispatch(stmt->body());
}

void ScopeBuilder::visit_block_expr(const NodePtr<BlockExpr>& expr) {
    const auto scope = symbols_.create_scope(
        ScopeType::Block, current_scope_, current_func_);
    expr->block_scope(scope);

    auto exit_scope = enter_scope(scope);
    visit_expr(expr);
}

void ScopeBuilder::visit_var_expr(const NodePtr<VarExpr>& expr) {
    expr->surrounding_scope(current_scope_);
    visit_expr(expr);
}

void ScopeBuilder::visit_node(const NodePtr<>& node) {
    dispatch_children(node);
}

void ScopeBuilder::add_decl(const NodePtr<Decl>& decl) {
    HAMMER_ASSERT_NOT_NULL(decl);
    HAMMER_ASSERT(current_scope_, "Not inside a scope.");

    auto entry = current_scope_->insert(decl);
    if (!entry) {
        diag_.reportf(Diagnostics::Error, decl->start(),
            "The name '{}' has already been declared in this scope.",
            strings_.value(decl->name()));
        return;
    }

    decl->declared_symbol(entry);
}

ResetValue<ScopePtr> ScopeBuilder::enter_scope(ScopePtr new_scope) {
    ScopePtr old_scope = std::exchange(current_scope_, std::move(new_scope));
    return {current_scope_, std::move(old_scope)};
}

ResetValue<NodePtr<FuncDecl>>
ScopeBuilder::enter_func(NodePtr<FuncDecl> new_func) {
    NodePtr<FuncDecl> old_func = std::exchange(
        current_func_, std::move(new_func));
    return {current_func_, std::move(old_func)};
}

void ScopeBuilder::dispatch_children(const NodePtr<>& node) {
    if (node) {
        traverse_children(node, [&](auto&& child) { dispatch(child); });
    }
}

} // namespace hammer::compiler
