#include "tiro/semantics/scope_builder.hpp"

#include "tiro/compiler/diagnostics.hpp"
#include "tiro/semantics/symbol_table.hpp"

namespace tiro {

ScopeBuilder::ScopeBuilder(
    SymbolTable& symbols, StringTable& strings, Diagnostics& diag)
    : symbols_(symbols)
    , strings_(strings)
    , diag_(diag)
    , global_scope_(symbols.root())
    , current_scope_(global_scope_) {}

ScopeBuilder::~ScopeBuilder() {}

void ScopeBuilder::dispatch(AstNode* node) {
    if (node && !node->has_error())
        visit(TIRO_NN(node), *this);
}

void ScopeBuilder::visit_file(NotNull<AstFile*> file) {
    auto scope = register_scope(ScopeType::File, file);
    auto exit = enter_scope(scope);
    dispatch_children(file);
}

void ScopeBuilder::visit_func_decl(NotNull<AstFuncDecl*> func) {
    // TODO: Decls should always have a valid name (?). Dont make anon functions decls.
    // TODO: Symbol names and ast nodes
    if (func->name()) {
        register_decl(func, SymbolType::Function, func->name(),
            SymbolKey::for_node(func->id()));
    }

    auto scope = register_scope(ScopeType::Function, func);

    auto exit_func = enter_func(func);
    auto exit_scope = enter_scope(scope);
    for (auto param : func->params())
        dispatch(param);

    dispatch_block(func->body());
}

void ScopeBuilder::visit_param_decl(NotNull<AstParamDecl*> param) {
    // TODO: Symbol names and ast nodes
    if (param->name()) {
        register_decl(param, SymbolType::Parameter, param->name(),
            SymbolKey::for_node(param->id()));
    }

    dispatch_children(param);
}

void ScopeBuilder::visit_var_decl(NotNull<AstVarDecl*> var) {
    for (auto binding : var->bindings())
        dispatch(binding);
}

void ScopeBuilder::visit_decl([[maybe_unused]] NotNull<AstDecl*> decl) {
    // Must not be called. Special visit functions are needed for every subtype of AstDecl.
    TIRO_UNREACHABLE("Failed to overwrite declaration type.");
}

void ScopeBuilder::visit_tuple_binding(NotNull<AstTupleBinding*> binding) {
    const u32 name_count = checked_cast<u32>(binding->names().size());
    for (u32 i = 0; i < name_count; ++i) {
        auto name = binding->names()[i];
        register_decl(binding, SymbolType::Variable, name,
            SymbolKey::for_element(binding->id(), i));
    }

    dispatch_children(binding);
}

void ScopeBuilder::visit_var_binding(NotNull<AstVarBinding*> binding) {
    if (binding->name()) {
        register_decl(binding, SymbolType::Variable, binding->name(),
            SymbolKey::for_node(binding->id()));
    }

    dispatch_children(binding);
}

void ScopeBuilder::visit_binding(NotNull<AstBinding*> binding) {
    // Must not be called. Special visit functions are needed for every subtype of AstBinding.
    TIRO_UNREACHABLE("Failed to overwrite binding type.");
}

void ScopeBuilder::visit_for_stmt(NotNull<AstForStmt*> stmt) {
    auto scope = register_scope(ScopeType::ForStatement, stmt);
    auto exit = enter_scope(scope);
    dispatch(stmt->decl());
    dispatch(stmt->cond());
    dispatch(stmt->step());
    dispatch_block(stmt->body());
}

void ScopeBuilder::visit_while_stmt(NotNull<AstWhileStmt*> stmt) {
    dispatch(stmt->cond());
    dispatch_block(stmt->body());
}

void ScopeBuilder::visit_block_expr(NotNull<AstBlockExpr*> expr) {
    auto scope = register_scope(ScopeType::Block, expr);
    auto exit = enter_scope(scope);
    visit_expr(expr);
}

void ScopeBuilder::visit_var_expr(NotNull<AstVarExpr*> expr) {
    surrounding_scope_[expr->id()] = current_scope_;
    visit_expr(expr);
}

void ScopeBuilder::visit_expr(NotNull<AstExpr*> expr) {
    visit_node(expr);
}

void ScopeBuilder::visit_node(NotNull<AstNode*> node) {
    dispatch_children(node);
}

SymbolId ScopeBuilder::register_decl(NotNull<AstNode*> node, SymbolType type,
    InternedString name, const SymbolKey& key) {
    TIRO_DEBUG_ASSERT(current_scope_, "Not inside a scope.");
    TIRO_DEBUG_ASSERT(
        node->id() == key.node(), "Symbol key and node must be consistent.");

    const auto scope_type = symbols_[current_scope_]->type();
    switch (type) {
    case SymbolType::Import:
        TIRO_DEBUG_ASSERT(scope_type == ScopeType::File,
            "Imports are only allowed at file scope.");
        break;
    case SymbolType::Type:
        TIRO_DEBUG_ASSERT(false, "Types are not implemented yet.");
        break;
    case SymbolType::Function:
        break; // allowed everywhere
    case SymbolType::Parameter:
        TIRO_DEBUG_ASSERT(scope_type == ScopeType::Function,
            "Parameters are only allowed at function scope.");
        break;
    case SymbolType::Variable:
        TIRO_DEBUG_ASSERT(scope_type == ScopeType::File
                              || scope_type == ScopeType::ForStatement
                              || scope_type == ScopeType::Block,
            "Variables are not allowed in this context.");
        break;
    }

    auto sym_id = symbols_.register_decl(
        Symbol(current_scope_, type, name, key));
    if (!sym_id) {
        node->has_error(true);
        diag_.reportf(Diagnostics::Error, node->source(),
            "The name '{}' has already been declared in this scope.",
            strings_.value(name));
    }
    return sym_id;
}

ScopeId ScopeBuilder::register_scope(ScopeType type, NotNull<AstNode*> node) {
    TIRO_DEBUG_ASSERT(current_scope_, "Must have a current scope.");
    return symbols_.register_scope(current_scope_, type, node->id());
}

ResetValue<ScopeId> ScopeBuilder::enter_scope(ScopeId new_scope) {
    auto old_scope = std::exchange(current_scope_, std::move(new_scope));
    return {current_scope_, std::move(old_scope)};
}

ResetValue<AstFuncDecl*>
ScopeBuilder::enter_func(NotNull<AstFuncDecl*> new_func) {
    auto old_func = std::exchange(current_func_, new_func);
    return {current_func_, std::move(old_func)};
}

// Called to ensure that the child is always wrapped in a fresh block scope.
void ScopeBuilder::dispatch_block(AstExpr* node) {
    if (node && !is_instance<AstBlockExpr>(node)) {
        auto scope = register_scope(ScopeType::Block, TIRO_NN(node));
        auto exit = enter_scope(scope);
        dispatch(node);
    } else {
        dispatch(node);
    }
}

void ScopeBuilder::dispatch_children(NotNull<AstNode*> node) {
    node->traverse_children([&](AstNode* child) { dispatch(child); });
}

} // namespace tiro
