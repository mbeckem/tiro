#include "tiro/semantics/scope_builder.hpp"

#include "tiro/compiler/diagnostics.hpp"

namespace tiro {

ScopeBuilder::ScopeBuilder(SymbolTable& symbols, StringTable& strings,
    Diagnostics& diag, ScopePtr global_scope)
    : symbols_(symbols)
    , strings_(strings)
    , diag_(diag)
    , global_scope_(global_scope)
    , current_scope_(nullptr) {}

ScopeBuilder::~ScopeBuilder() {}

void ScopeBuilder::dispatch(Node* node) {
    if (node && !node->has_error()) {
        visit(TIRO_NN(node), *this);
    }
}

void ScopeBuilder::visit_root(Root* root) {
    root->root_scope(global_scope_);

    auto exit_scope = enter_scope(global_scope_);
    dispatch_children(root);
}

void ScopeBuilder::visit_file(File* file) {
    const auto scope = symbols_.create_scope(
        ScopeType::File, current_scope_, current_func_);
    file->file_scope(scope);

    auto exit_scope = enter_scope(scope);
    dispatch_children(file);
}

void ScopeBuilder::visit_func_decl(FuncDecl* func) {
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

void ScopeBuilder::visit_decl(Decl* decl) {
    // TODO: Decls should always have a valid name.
    if (decl->name())
        add_decl(decl);

    dispatch_children(decl);
}

void ScopeBuilder::visit_for_stmt(ForStmt* stmt) {
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

void ScopeBuilder::visit_while_stmt(WhileStmt* stmt) {
    const auto body_scope = symbols_.create_scope(
        ScopeType::LoopBody, current_scope_, current_func_);
    stmt->body_scope(body_scope);

    dispatch(stmt->condition());

    auto exit_body_scope = enter_scope(body_scope);
    dispatch(stmt->body());
}

void ScopeBuilder::visit_block_expr(BlockExpr* expr) {
    const auto scope = symbols_.create_scope(
        ScopeType::Block, current_scope_, current_func_);
    expr->block_scope(scope);

    auto exit_scope = enter_scope(scope);
    visit_expr(expr);
}

void ScopeBuilder::visit_expr(Expr* expr) {
    expr->surrounding_scope(current_scope_);
    visit_node(expr);
}

void ScopeBuilder::visit_node(Node* node) {
    dispatch_children(node);
}

void ScopeBuilder::add_decl(Decl* decl) {
    TIRO_ASSERT_NOT_NULL(decl);
    TIRO_ASSERT(current_scope_, "Not inside a scope.");

    const auto scope_type = current_scope_->type();
    const auto symbol_type = [&]() {
        switch (decl->type()) {
        case NodeType::FuncDecl:
            // TODO handle local function declarations once implemented.
            TIRO_ASSERT(scope_type == ScopeType::File,
                "Functions must be at file scope.");
            return SymbolType::Function;
        case NodeType::ImportDecl:
            // TODO handle local import declarations once implemented.
            TIRO_ASSERT(scope_type == ScopeType::File,
                "Imports must be at file scope.");
            return SymbolType::Import;
        case NodeType::ParamDecl:
            TIRO_ASSERT(current_scope_->function() != nullptr,
                "Must be inside a function.");
            TIRO_ASSERT(scope_type == ScopeType::Parameters,
                "Parameters are only allowed in function scopes.");
            return SymbolType::ParameterVar;
        case NodeType::VarDecl:
            return scope_type == ScopeType::File ? SymbolType::ModuleVar
                                                 : SymbolType::LocalVar;

        default:
            TIRO_UNREACHABLE("Invalid declaration type.");
        }
    }();

    auto entry = current_scope_->insert(symbol_type, ref(decl));
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

ResetValue<NodePtr<FuncDecl>> ScopeBuilder::enter_func(FuncDecl* new_func) {
    NodePtr<FuncDecl> old_func = std::exchange(current_func_, ref(new_func));
    return {current_func_, std::move(old_func)};
}

void ScopeBuilder::dispatch_children(Node* node) {
    if (node) {
        traverse_children(
            TIRO_NN(node), [&](auto&& child) { dispatch(child); });
    }
}

} // namespace tiro
