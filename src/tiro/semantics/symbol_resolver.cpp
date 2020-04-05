#include "tiro/semantics/symbol_resolver.hpp"

#include "tiro/compiler/diagnostics.hpp"
#include "tiro/semantics/analyzer.hpp"
#include "tiro/semantics/symbol_table.hpp"

namespace tiro {

SymbolResolver::SymbolResolver(
    SymbolTable& symbols, StringTable& strings, Diagnostics& diag)
    : symbols_(symbols)
    , strings_(strings)
    , diag_(diag) {}

SymbolResolver::~SymbolResolver() {}

void SymbolResolver::dispatch(Node* node) {
    if (node && !node->has_error()) {
        visit(TIRO_NN(node), *this);
    }
}

void SymbolResolver::visit_binding(Binding* binding) {
    // Var is not active in initializer
    dispatch(binding->init());
    visit_vars(TIRO_NN(binding), [&](VarDecl* var) { dispatch(var); });
}

void SymbolResolver::visit_decl(Decl* decl) {
    // TODO classes will also be active in their bodies
    const bool active_in_children = isa<FuncDecl>(decl);
    if (active_in_children) {
        activate(decl);
        visit_node(decl);
    } else {
        visit_node(decl);
        activate(decl);
    }
}

void SymbolResolver::visit_file(File* file) {
    // Function declarations in file scope are always active.
    // TODO: Variables / constants / classes
    // TODO: can use the file scope for this instead
    auto scope = file->file_scope();
    TIRO_DEBUG_NOT_NULL(scope);

    for (const auto& entry : scope->entries()) {
        if (isa<FuncDecl>(entry->decl())) {
            entry->active(true);
        }
    }

    visit_node(file);
}

void SymbolResolver::visit_var_expr(VarExpr* expr) {
    auto expr_scope = expr->surrounding_scope();
    TIRO_CHECK(expr_scope, "Scope was not set for this expression.");
    TIRO_CHECK(expr->name(), "Variable reference without a name.");

    if (expr->resolved_symbol())
        return;

    auto [decl_entry, decl_scope] = expr_scope->find(expr->name());
    if (!decl_entry) {
        diag_.reportf(Diagnostics::Error, expr->start(),
            "Undefined symbol: '{}'.", strings_.value(expr->name()));
        expr->has_error(true);
        return;
    }

    // Variables and constants at module scope are not captured.
    const bool can_capture = decl_scope->type() != ScopeType::File
                             && decl_scope->type() != ScopeType::Global;
    if (can_capture && decl_scope->function() != expr_scope->function()
        && expr_scope->is_child_of(decl_scope)) {
        // Expr references something within an outer function
        decl_entry->captured(true);
    }

    if (!decl_entry->active()) {
        diag_.reportf(Diagnostics::Error, expr->start(),
            "Symbol '{}' referenced before it became active in the current "
            "scope.",
            strings_.value(expr->name()));
        expr->has_error(true);
        return;
    }

    expr->resolved_symbol(decl_entry);
    visit_expr(expr);
}

void SymbolResolver::visit_node(Node* node) {
    dispatch_children(node);
}

void SymbolResolver::activate(Decl* decl) {
    auto decl_entry = decl->declared_symbol();
    if (!decl_entry) // TODO there should always be a declared symbol in the future
        return;
    decl_entry->active(true);
}

void SymbolResolver::dispatch_children(Node* node) {
    if (node) {
        traverse_children(
            TIRO_NN(node), [&](auto&& child) { dispatch(child); });
    }
}

} // namespace tiro
