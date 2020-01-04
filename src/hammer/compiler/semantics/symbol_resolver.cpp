#include "hammer/compiler/semantics/symbol_resolver.hpp"

#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/semantics/symbol_table.hpp"

namespace hammer::compiler {

SymbolResolver::SymbolResolver(
    SymbolTable& symbols, StringTable& strings, Diagnostics& diag)
    : symbols_(symbols)
    , strings_(strings)
    , diag_(diag) {}

SymbolResolver::~SymbolResolver() {}

void SymbolResolver::dispatch(const NodePtr<>& node) {
    if (node && !node->has_error()) {
        visit(node, *this);
    }
}

void SymbolResolver::visit_decl(const NodePtr<Decl>& decl) {
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

void SymbolResolver::visit_file(const NodePtr<File>& file) {
    // Function declarations in file scope are always active.
    // TODO: Variables / constants / classes
    // TODO: can use the file scope for this instead
    auto scope = file->file_scope();
    HAMMER_ASSERT_NOT_NULL(scope);

    for (auto entry : scope->entries()) {
        if (isa<FuncDecl>(entry->decl())) {
            entry->active(true);
        }
    }

    visit_node(file);
}

void SymbolResolver::visit_var_expr(const NodePtr<VarExpr>& expr) {
    auto expr_scope = expr->surrounding_scope();
    HAMMER_CHECK(expr_scope, "Scope was not set for this expression.");
    HAMMER_CHECK(expr->resolved_symbol() == nullptr,
        "Symbol has already been resolved.");
    HAMMER_CHECK(expr->name(), "Variable reference without a name.");

    auto [decl_entry, decl_scope] = expr_scope->find(expr->name());
    if (!decl_entry) {
        diag_.reportf(Diagnostics::Error, expr->start(),
            "Undefined symbol: '{}'.", strings_.value(expr->name()));
        expr->has_error(true);
        return;
    }

    if (decl_scope->function() != expr_scope->function()
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

void SymbolResolver::visit_node(const NodePtr<>& node) {
    dispatch_children(node);
}

void SymbolResolver::activate(const NodePtr<Decl>& decl) {
    auto decl_entry = decl->declared_symbol();
    if (!decl_entry) // TODO there should always be a declared symbol in the future
        return;
    decl_entry->active(true);
}

void SymbolResolver::dispatch_children(const NodePtr<>& node) {
    if (node) {
        traverse_children(node, [&](auto&& child) { dispatch(child); });
    }
}

} // namespace hammer::compiler