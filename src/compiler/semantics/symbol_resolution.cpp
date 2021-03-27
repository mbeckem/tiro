#include "compiler/semantics/symbol_resolution.hpp"

#include "compiler/ast/ast.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/reset_value.hpp"
#include "compiler/semantics/analysis.hpp"

#include "absl/container/flat_hash_map.h"

namespace tiro {

namespace {

// Maps an AST node id (of a symbol reference) to the surrounding scope.
// Symbols are resolved after all declarations have been processed.
class SurroundingScopes final {
public:
    void add(AstId node, ScopeId surrounding_scope) {
        TIRO_DEBUG_ASSERT(node, "Invalid node.");
        TIRO_DEBUG_ASSERT(surrounding_scope, "Invalid scope.");
        TIRO_DEBUG_ASSERT(scopes_.find(node) == scopes_.end(),
            "A surrounding scope for that node was already registered.");
        scopes_.emplace(node, surrounding_scope);
    }

    ScopeId find(AstId node) const {
        if (auto pos = scopes_.find(node); pos != scopes_.end())
            return pos->second;
        return ScopeId();
    }

    ScopeId get(AstId node) const {
        auto scope = find(node);
        TIRO_DEBUG_ASSERT(scope, "Failed to find scope for ast node.");
        return scope;
    }

private:
    absl::flat_hash_map<AstId, ScopeId, UseHasher> scopes_;
};

/// The scope builder assembles the tree of lexical scopes and discovers all declarations.
/// Declarations encountered while walking down the tree are registered with the currently active scope.
/// References to names are not yet resolved, because some items may be referenced before their declaration
/// has been observed.
class ScopeBuilder final : public DefaultNodeVisitor<ScopeBuilder> {
public:
    explicit ScopeBuilder(
        SurroundingScopes& scopes, SymbolTable& table, StringTable& strings, Diagnostics& diag);
    ~ScopeBuilder();

    ScopeBuilder(const ScopeBuilder&) = delete;
    ScopeBuilder& operator=(const ScopeBuilder&) = delete;

    // Entry point. Visits the concrete type of the node (if it is valid).
    void dispatch(AstNode* node);

    void visit_file(NotNull<AstFile*> file) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_import_decl(NotNull<AstImportDecl*> imp) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_func_decl(NotNull<AstFuncDecl*> func) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_param_decl(NotNull<AstParamDecl*> param) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_var_decl(NotNull<AstVarDecl*> var) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_decl(NotNull<AstDecl*> decl) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_binding(NotNull<AstBinding*> binding) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_for_stmt(NotNull<AstForStmt*> stmt) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_for_each_stmt(NotNull<AstForEachStmt*> stmt) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_while_stmt(NotNull<AstWhileStmt*> stmt) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_block_expr(NotNull<AstBlockExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_var_expr(NotNull<AstVarExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_expr(NotNull<AstExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_node(NotNull<AstNode*> node) TIRO_NODE_VISITOR_OVERRIDE;

    void handle_decl_modifiers(NotNull<AstDecl*> decl);

private:
    enum Mutability { Mutable, Constant };

    // Add a declaration to the symbol table (within the current scope).
    SymbolId register_decl(
        NotNull<AstNode*> node, InternedString name, Mutability mutability, const SymbolData& data);

    // Add a scope as a child of the current scope.
    ScopeId register_scope(ScopeType type, NotNull<AstNode*> node);

    // Lookup the symbol for the given node and mark it as exported.
    bool mark_exported(NotNull<const AstNode*> node);

    ResetValue<ScopeId> enter_scope(ScopeId new_scope);
    ResetValue<SymbolId> enter_func(SymbolId new_func);

    // Called to ensure that the child is always wrapped in a fresh block scope.
    // This is kinda ugly, it would be nice if a ast node could define multiple scopes (it is
    // is a 1-to-1 mapping atm).
    void dispatch_block(AstExpr* node);

    // Visits the given node and makes sure that it's scope is marked as a loop body.
    void dispatch_loop_body(AstExpr* node);

    // Recurse into all children of the given node.
    void dispatch_children(NotNull<AstNode*> node);

private:
    SurroundingScopes& scopes_;
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;

    ScopeId global_scope_;
    ScopeId current_scope_;
    SymbolId current_func_;
};

/// Links symbol references to declared symbols. Uses the intermediate results
/// from the ScopeBuilder pass to resolve references within their scope.
/// Errors are raised when references are illegal (e.g. referencing a variable before its definition).
class SymbolResolver final : public DefaultNodeVisitor<SymbolResolver> {
public:
    explicit SymbolResolver(const SurroundingScopes& scopes, SymbolTable& table,
        const StringTable& strings, Diagnostics& diag);
    ~SymbolResolver();

    SymbolResolver(const SymbolResolver&) = delete;
    SymbolResolver& operator=(const SymbolResolver&) = delete;

    // Entry point. Visits the concrete type of the node (if it is valid).
    void dispatch(AstNode* node);

    void visit_import_decl(NotNull<AstImportDecl*> item) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_func_decl(NotNull<AstFuncDecl*> func) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_param_decl(NotNull<AstParamDecl*> param) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_var_decl(NotNull<AstVarDecl*> var) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_decl(NotNull<AstDecl*> decl) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_file(NotNull<AstFile*> file) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_for_each_stmt(NotNull<AstForEachStmt*> stmt) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_var_expr(NotNull<AstVarExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_node(NotNull<AstNode*> node) TIRO_NODE_VISITOR_OVERRIDE;

private:
    struct ActivateVarVisitor {
        SymbolResolver& self;

        void visit_var_binding_spec(NotNull<AstVarBindingSpec*> v) {
            self.activate(TIRO_NN(v->name()));
        }

        void visit_tuple_binding_spec(NotNull<AstTupleBindingSpec*> t) {
            for (auto name : t->names()) {
                self.activate(TIRO_NN(name));
            }
        }
    };

    void activate_var(NotNull<AstBindingSpec*> spec) { visit(spec, ActivateVarVisitor{*this}); }

    // Activate the declaration associated with this node. Referencing an inactive
    // symbol results in an error.
    void activate(NotNull<AstNode*> node);

    // Recurse into all children of the given node.
    void dispatch_children(NotNull<AstNode*> node);

private:
    const SurroundingScopes& scopes_;
    SymbolTable& table_;
    const StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace

static InternedString imported_path(NotNull<const AstImportDecl*> imp, StringTable& strings) {
    std::string joined_string;
    for (auto element : imp->path()) {
        if (!joined_string.empty())
            joined_string += ".";
        joined_string += strings.value(element);
    }
    return strings.insert(joined_string);
}

static void
visit_binding_names(NotNull<AstBindingSpec*> spec, FunctionRef<void(AstIdentifier*)> cb) {
    struct SpecVisitor {
        FunctionRef<void(AstIdentifier*)> cb;

        void visit_var_binding_spec(NotNull<AstVarBindingSpec*> var) { cb(var->name()); }

        void visit_tuple_binding_spec(NotNull<AstTupleBindingSpec*> tuple) {
            for (auto name : tuple->names())
                cb(name);
        }
    };

    visit(spec, SpecVisitor{cb});
}

ScopeBuilder::ScopeBuilder(
    SurroundingScopes& scopes, SymbolTable& table, StringTable& strings, Diagnostics& diag)
    : scopes_(scopes)
    , symbols_(table)
    , strings_(strings)
    , diag_(diag)
    , global_scope_(table.root())
    , current_scope_(global_scope_) {}

ScopeBuilder::~ScopeBuilder() {}

void ScopeBuilder::dispatch(AstNode* node) {
    if (node && !node->has_error())
        visit(TIRO_NN(node), *this);
}

void ScopeBuilder::visit_file(NotNull<AstFile*> file) {
    auto scope_id = register_scope(ScopeType::File, file);
    auto exit = enter_scope(scope_id);
    dispatch_children(file);
}

void ScopeBuilder::visit_import_decl(NotNull<AstImportDecl*> imp) {
    auto path = imported_path(imp, strings_);
    register_decl(imp, imp->name(), Constant, SymbolData::make_import(path));
    handle_decl_modifiers(imp);
}

void ScopeBuilder::visit_func_decl(NotNull<AstFuncDecl*> func) {
    auto symbol_id = register_decl(func, func->name(), Constant, SymbolData::make_function());
    auto exit_func = enter_func(symbol_id); // Scope creation references current function

    {
        auto scope = register_scope(ScopeType::Function, func);
        auto exit_scope = enter_scope(scope);

        for (auto param : func->params())
            dispatch(param);

        dispatch_block(func->body());
    }

    // Must be outside the function's scope.
    handle_decl_modifiers(func);
}

void ScopeBuilder::visit_param_decl(NotNull<AstParamDecl*> param) {
    register_decl(param, param->name(), Mutable, SymbolData::make_parameter());
    dispatch_children(param);
}

void ScopeBuilder::visit_var_decl(NotNull<AstVarDecl*> var) {
    for (auto binding : var->bindings())
        dispatch(binding);

    handle_decl_modifiers(var);
}

[[maybe_unused]] void ScopeBuilder::visit_decl([[maybe_unused]] NotNull<AstDecl*> decl) {
    // Must not be called. Special visit functions are needed for every subtype of AstDecl.
    TIRO_UNREACHABLE("Failed to overwrite declaration type.");
}

void ScopeBuilder::visit_binding(NotNull<AstBinding*> binding) {
    auto mutability = binding->is_const() ? Constant : Mutable;

    visit_binding_names(TIRO_NN(binding->spec()), [&](auto name) {
        register_decl(TIRO_NN(name), name->value(), mutability, SymbolData::make_variable());
    });

    dispatch(binding->init());
}

void ScopeBuilder::visit_for_stmt(NotNull<AstForStmt*> stmt) {
    auto scope_id = register_scope(ScopeType::ForStatement, stmt);
    auto exit = enter_scope(scope_id);
    dispatch(stmt->decl());
    dispatch(stmt->cond());
    dispatch(stmt->step());
    dispatch_loop_body(stmt->body());
}

void ScopeBuilder::visit_for_each_stmt(NotNull<AstForEachStmt*> stmt) {
    dispatch(stmt->expr());

    auto scope_id = register_scope(ScopeType::ForStatement, stmt);
    symbols_[scope_id].is_loop_scope(true);
    auto exit = enter_scope(scope_id);

    // The declared variable is part of the loop scope, this ensures that
    // it every closure inside a loop will observe a new variable.
    visit_binding_names(TIRO_NN(stmt->spec()), [&](auto name) {
        register_decl(TIRO_NN(name), name->value(), Constant, SymbolData::make_variable());
    });

    dispatch(stmt->body());
}

void ScopeBuilder::visit_while_stmt(NotNull<AstWhileStmt*> stmt) {
    dispatch(stmt->cond());
    dispatch_loop_body(stmt->body());
}

void ScopeBuilder::visit_block_expr(NotNull<AstBlockExpr*> expr) {
    auto scope_id = register_scope(ScopeType::Block, expr);
    auto exit = enter_scope(scope_id);
    visit_expr(expr);
}

void ScopeBuilder::visit_var_expr(NotNull<AstVarExpr*> expr) {
    scopes_.add(expr->id(), current_scope_);
    visit_expr(expr);
}

void ScopeBuilder::visit_expr(NotNull<AstExpr*> expr) {
    visit_node(expr);
}

void ScopeBuilder::visit_node(NotNull<AstNode*> node) {
    dispatch_children(node);
}

void ScopeBuilder::handle_decl_modifiers(NotNull<AstDecl*> decl) {
    if (decl->has_error())
        return;

    for (auto modifier : decl->modifiers()) {
        if (is_instance<AstExportModifier>(modifier)) {
            const auto& scope = symbols_[current_scope_];
            if (scope.type() != ScopeType::File) {
                diag_.reportf(
                    Diagnostics::Error, decl->range(), "Exports are only allowed at file scope.");
                return;
            }

            // Find symbols defined by this declaration and mark them as exported.
            struct ExportedItemVisitor {
                ScopeBuilder& self;

                void visit_param_decl([[maybe_unused]] NotNull<AstParamDecl*> param) {
                    TIRO_ERROR("Parameters cannot be exported.");
                }

                void visit_import_decl([[maybe_unused]] NotNull<AstImportDecl*> imp) {
                    TIRO_ERROR("Exported import items are not implemented yet."); // FIXME
                }

                void visit_func_decl(NotNull<AstFuncDecl*> func) { self.mark_exported(func); }

                void visit_var_decl(NotNull<AstVarDecl*> var) {
                    for (auto binding : var->bindings()) {
                        if (!binding || binding->has_error())
                            continue;

                        auto spec = binding->spec();
                        if (!spec || spec->has_error())
                            continue;

                        visit_binding_names(
                            TIRO_NN(spec), [&](auto name) { self.mark_exported(TIRO_NN(name)); });
                    }
                }
            };
            visit(decl, ExportedItemVisitor{*this});
        }
    }
}

SymbolId ScopeBuilder::register_decl(
    NotNull<AstNode*> node, InternedString name, Mutability mutability, const SymbolData& data) {
    TIRO_DEBUG_ASSERT(current_scope_, "Not inside a scope.");

    [[maybe_unused]] const auto scope_type = symbols_[current_scope_].type();
    switch (data.type()) {
    case SymbolType::Import:
        TIRO_DEBUG_ASSERT(scope_type == ScopeType::File, "Imports are only allowed at file scope.");
        break;
    case SymbolType::TypeSymbol:
        TIRO_DEBUG_ASSERT(false, "Types are not implemented yet.");
        break;
    case SymbolType::Function:
        break; // allowed everywhere
    case SymbolType::Parameter:
        TIRO_DEBUG_ASSERT(
            scope_type == ScopeType::Function, "Parameters are only allowed at function scope.");
        break;
    case SymbolType::Variable:
        TIRO_DEBUG_ASSERT(scope_type == ScopeType::File || scope_type == ScopeType::ForStatement
                              || scope_type == ScopeType::Block,
            "Variables are not allowed in this context.");
        break;
    }

    auto sym_id = symbols_.register_decl(Symbol(current_scope_, name, node->id(), data));
    if (!sym_id) {
        node->has_error(true);
        diag_.reportf(Diagnostics::Error, node->range(),
            "The name '{}' has already been declared in this scope.", strings_.dump(name));

        // Generate an anonymous symbol to ensure that the analyzer can continue.
        sym_id = symbols_.register_decl(Symbol(current_scope_, InternedString(), node->id(), data));
        TIRO_DEBUG_ASSERT(sym_id, "Anonymous symbols can always be created.");
    }

    auto& sym = symbols_[sym_id];
    sym.is_const(mutability == Constant);
    return sym_id;
}

ScopeId ScopeBuilder::register_scope(ScopeType type, NotNull<AstNode*> node) {
    TIRO_DEBUG_ASSERT(current_scope_, "Must have a current scope.");
    return symbols_.register_scope(current_scope_, current_func_, type, node->id());
}

bool ScopeBuilder::mark_exported(NotNull<const AstNode*> node) {
    auto symbol_id = symbols_.find_decl(node->id());
    TIRO_CHECK(symbol_id, "Exported item did not declare a symbol.");

    auto& symbol = symbols_[symbol_id];
    if (!symbol.name()) {
        diag_.reportf(Diagnostics::Error, node->range(), "An anonymous symbol cannot be exported.");
        return false;
    }

    if (!symbol.is_const()) {
        diag_.reportf(Diagnostics::Error, node->range(),
            "The symbol '{}' must be a constant in order to be exported.",
            strings_.value(symbol.name()));
        return false;
    }

    symbol.exported(true);
    return true;
}

ResetValue<ScopeId> ScopeBuilder::enter_scope(ScopeId new_scope) {
    return replace_value(current_scope_, new_scope);
}

ResetValue<SymbolId> ScopeBuilder::enter_func(SymbolId new_func) {
    return replace_value(current_func_, new_func);
}

void ScopeBuilder::dispatch_block(AstExpr* node) {
    if (node && !is_instance<AstBlockExpr>(node)) {
        auto scope = register_scope(ScopeType::Block, TIRO_NN(node));
        auto exit = enter_scope(scope);
        dispatch(node);
    } else {
        dispatch(node);
    }
}

void ScopeBuilder::dispatch_loop_body(AstExpr* node) {
    if (!node || node->has_error())
        return;

    dispatch_block(node);

    auto scope_id = symbols_.get_scope(node->id());
    symbols_[scope_id].is_loop_scope(true);
}

void ScopeBuilder::dispatch_children(NotNull<AstNode*> node) {
    node->traverse_children([&](AstNode* child) { dispatch(child); });
}

SymbolResolver::SymbolResolver(const SurroundingScopes& scopes, SymbolTable& table,
    const StringTable& strings, Diagnostics& diag)
    : scopes_(scopes)
    , table_(table)
    , strings_(strings)
    , diag_(diag) {}

SymbolResolver::~SymbolResolver() {}

void SymbolResolver::dispatch(AstNode* node) {
    if (node && !node->has_error()) {
        visit(TIRO_NN(node), *this);
    }
}

void SymbolResolver::visit_import_decl(NotNull<AstImportDecl*> item) {
    dispatch_children(item);
    activate(item);
}

void SymbolResolver::visit_func_decl(NotNull<AstFuncDecl*> func) {
    // Function names are visible from their bodies.
    activate(func);
    dispatch_children(func);
}

void SymbolResolver::visit_param_decl(NotNull<AstParamDecl*> param) {
    dispatch_children(param);
    activate(param);
}

void SymbolResolver::visit_var_decl(NotNull<AstVarDecl*> var) {
    // Var is not active in initializer
    for (AstBinding* binding : var->bindings()) {
        if (!binding || binding->has_error())
            continue;

        dispatch(binding->init());
        activate_var(TIRO_NN(binding->spec()));
    }
}

[[maybe_unused]] void SymbolResolver::visit_decl([[maybe_unused]] NotNull<AstDecl*> decl) {
    // Must not be called. Special visit functions are needed for every subtype of AstDecl.
    TIRO_UNREACHABLE("Failed to overwrite decl type.");
}

void SymbolResolver::visit_file(NotNull<AstFile*> file) {
    // Function declarations in file scope are always active.
    const auto& scope = table_[table_.get_scope(file->id())];
    for (auto symbol_id : scope.entries()) {
        auto& symbol = table_[symbol_id];

        // TODO: Variables / constants / classes visible?
        if (symbol.type() == SymbolType::Function) {
            symbol.active(true);
        }
    }

    dispatch_children(file);
}

void SymbolResolver::visit_for_each_stmt(NotNull<AstForEachStmt*> stmt) {
    // Var is not active in the container expression
    dispatch(stmt->expr());
    activate_var(TIRO_NN(stmt->spec()));
    dispatch(stmt->body());
}

void SymbolResolver::visit_var_expr(NotNull<AstVarExpr*> expr) {
    TIRO_CHECK(expr->name(), "Variable reference without a name.");

    auto expr_scope_id = scopes_.get(expr->id());
    auto [decl_scope_id, decl_symbol_id] = table_.find_name(expr_scope_id, expr->name());

    if (!decl_scope_id || !decl_symbol_id) {
        diag_.reportf(Diagnostics::Error, expr->range(), "Undefined symbol: '{}'.",
            strings_.value(expr->name()));
        expr->has_error(true);
        return;
    }

    auto expr_scope = table_.ptr_to(expr_scope_id);
    auto decl_scope = table_.ptr_to(decl_scope_id);
    auto decl_symbol = table_.ptr_to(decl_symbol_id);

    // Only symbols that are active by now can be referenced.
    if (!decl_symbol->active()) {
        diag_.reportf(Diagnostics::Error, expr->range(),
            "Symbol '{}' referenced before it became active in the current "
            "scope.",
            strings_.value(expr->name()));
        expr->has_error(true);
        return;
    }

    // Mark symbols as captured if they are being referenced from a nested function.
    // Variables and constants at module scope are not captured.
    if (!decl_symbol->captured()) {
        const bool can_capture = decl_scope->type() != ScopeType::File
                                 && decl_scope->type() != ScopeType::Global;
        if (can_capture && decl_scope->function() != expr_scope->function()
            && table_.is_strict_ancestor(decl_scope_id, expr_scope_id)) {
            decl_symbol->captured(true);
        }
    }

    table_.register_ref(expr->id(), decl_symbol_id);
    dispatch_children(expr);
}

void SymbolResolver::visit_node(NotNull<AstNode*> node) {
    dispatch_children(node);
}

void SymbolResolver::activate(NotNull<AstNode*> node) {
    auto symbol_id = table_.get_decl(node->id());
    table_[symbol_id].active(true);
}

void SymbolResolver::dispatch_children(NotNull<AstNode*> node) {
    node->traverse_children([&](AstNode* child) { dispatch(child); });
}

void resolve_symbols(SemanticAst& ast, Diagnostics& diag) {
    SymbolTable& table = ast.symbols();
    StringTable& strings = ast.strings();
    SurroundingScopes scopes;

    // First pass: build scopes and register all declarations.
    {
        ScopeBuilder builder(scopes, table, strings, diag);
        builder.dispatch(ast.root());
    }

    // Second pass: link references to symbol declarations.
    {
        SymbolResolver resolver(scopes, table, strings, diag);
        resolver.dispatch(ast.root());
    }
}

} // namespace tiro
