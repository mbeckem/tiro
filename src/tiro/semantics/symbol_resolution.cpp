#include "tiro/semantics/symbol_resolution.hpp"

#include "tiro/ast/ast.hpp"
#include "tiro/compiler/diagnostics.hpp"
#include "tiro/compiler/reset_value.hpp"

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
    // TODO: Better container
    std::unordered_map<AstId, ScopeId, UseHasher> scopes_;
};

/// The scope builder assembles the tree of nested scopes.
class ScopeBuilder final : public DefaultNodeVisitor<ScopeBuilder> {
public:
    explicit ScopeBuilder(SurroundingScopes& scopes, SymbolTable& table,
        StringTable& strings, Diagnostics& diag);
    ~ScopeBuilder();

    ScopeBuilder(const ScopeBuilder&) = delete;
    ScopeBuilder& operator=(const ScopeBuilder&) = delete;

    // Entry point. Visits the concrete type of the node (if it is valid).
    void dispatch(AstNode* node);

    void visit_file(NotNull<AstFile*> file) TIRO_NODE_VISITOR_OVERRIDE;

    void
    visit_import_item(NotNull<AstImportItem*> imp) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_func_decl(NotNull<AstFuncDecl*> func) TIRO_NODE_VISITOR_OVERRIDE;

    void
    visit_param_decl(NotNull<AstParamDecl*> param) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_var_decl(NotNull<AstVarDecl*> var) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_decl(NotNull<AstDecl*> decl) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_tuple_binding(
        NotNull<AstTupleBinding*> binding) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_var_binding(
        NotNull<AstVarBinding*> binding) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_binding(NotNull<AstBinding*> binding) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_for_stmt(NotNull<AstForStmt*> stmt) TIRO_NODE_VISITOR_OVERRIDE;

    void
    visit_while_stmt(NotNull<AstWhileStmt*> stmt) TIRO_NODE_VISITOR_OVERRIDE;

    void
    visit_block_expr(NotNull<AstBlockExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_var_expr(NotNull<AstVarExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_expr(NotNull<AstExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_node(NotNull<AstNode*> node) TIRO_NODE_VISITOR_OVERRIDE;

private:
    // Add a declaration to the symbol table (within the current scope).
    SymbolId register_decl(NotNull<AstNode*> node, InternedString name,
        const SymbolKey& key, const SymbolData& data);

    // Add a scope as a child of the current scope.
    ScopeId register_scope(ScopeType type, NotNull<AstNode*> node);

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

// Links symbol references to declared symbols.
class SymbolResolver final : public DefaultNodeVisitor<SymbolResolver> {
public:
    explicit SymbolResolver(const SurroundingScopes& scopes, SymbolTable& table,
        const StringTable& strings, Diagnostics& diag);
    ~SymbolResolver();

    SymbolResolver(const SymbolResolver&) = delete;
    SymbolResolver& operator=(const SymbolResolver&) = delete;

    // Entry point. Visits the concrete type of the node (if it is valid).
    void dispatch(AstNode* node);

    void visit_func_decl(NotNull<AstFuncDecl*> func) TIRO_NODE_VISITOR_OVERRIDE;

    void
    visit_param_decl(NotNull<AstParamDecl*> param) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_var_decl(NotNull<AstVarDecl*> var) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_decl(NotNull<AstDecl*> decl) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_file(NotNull<AstFile*> file) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_var_expr(NotNull<AstVarExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_node(NotNull<AstNode*> node) TIRO_NODE_VISITOR_OVERRIDE;

private:
    // Activate the declaration associated with the key. Referencing an inactive
    // symbol results in an error.
    void activate(const SymbolKey& key);

    // Recurse into all children of the given node.
    void dispatch_children(NotNull<AstNode*> node);

private:
    const SurroundingScopes& scopes_;
    SymbolTable& table_;
    const StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace

static SymbolKey key(NotNull<const AstImportItem*> imp) {
    return SymbolKey::for_node(imp->id());
}

static SymbolKey key(NotNull<const AstParamDecl*> param) {
    return SymbolKey::for_node(param->id());
}

static SymbolKey key(NotNull<const AstFuncDecl*> func) {
    return SymbolKey::for_node(func->id());
}

static SymbolKey key(NotNull<const AstVarBinding*> var) {
    return SymbolKey::for_node(var->id());
}

static SymbolKey key(NotNull<const AstTupleBinding*> tuple, u32 index) {
    return SymbolKey::for_element(tuple->id(), index);
}

static InternedString
imported_path(NotNull<const AstImportItem*> imp, StringTable& strings) {
    std::string joined_string;
    for (auto element : imp->path()) {
        if (!joined_string.empty())
            joined_string += ".";
        joined_string += strings.value(element);
    }
    return strings.insert(joined_string);
}

ScopeBuilder::ScopeBuilder(SurroundingScopes& scopes, SymbolTable& table,
    StringTable& strings, Diagnostics& diag)
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
    auto scope = register_scope(ScopeType::File, file);
    auto exit = enter_scope(scope);
    dispatch_children(file);
}

void ScopeBuilder::visit_import_item(NotNull<AstImportItem*> imp) {
    auto path = imported_path(imp, strings_);
    register_decl(imp, imp->name(), key(imp), SymbolData::make_import(path));

    dispatch_children(imp);
}

void ScopeBuilder::visit_func_decl(NotNull<AstFuncDecl*> func) {
    auto symbol = register_decl(
        func, func->name(), key(func), SymbolData::make_function());

    auto scope = register_scope(ScopeType::Function, func);

    auto exit_func = enter_func(symbol);
    auto exit_scope = enter_scope(scope);
    for (auto param : func->params())
        dispatch(param);

    dispatch_block(func->body());
}

void ScopeBuilder::visit_param_decl(NotNull<AstParamDecl*> param) {
    register_decl(
        param, param->name(), key(param), SymbolData::make_parameter());
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

void ScopeBuilder::visit_tuple_binding(NotNull<AstTupleBinding*> tuple) {
    const u32 name_count = checked_cast<u32>(tuple->names().size());
    for (u32 i = 0; i < name_count; ++i) {
        auto name = tuple->names()[i];
        register_decl(tuple, name, key(tuple, i), SymbolData::make_variable());
    }

    dispatch_children(tuple);
}

void ScopeBuilder::visit_var_binding(NotNull<AstVarBinding*> var) {
    register_decl(var, var->name(), key(var), SymbolData::make_variable());
    dispatch_children(var);
}

void ScopeBuilder::visit_binding(
    [[maybe_unused]] NotNull<AstBinding*> binding) {
    // Must not be called. Special visit functions are needed for every subtype of AstBinding.
    TIRO_UNREACHABLE("Failed to overwrite binding type.");
}

void ScopeBuilder::visit_for_stmt(NotNull<AstForStmt*> stmt) {
    auto scope = register_scope(ScopeType::ForStatement, stmt);
    auto exit = enter_scope(scope);
    dispatch(stmt->decl());
    dispatch(stmt->cond());
    dispatch(stmt->step());
    dispatch_loop_body(stmt->body());
}

void ScopeBuilder::visit_while_stmt(NotNull<AstWhileStmt*> stmt) {
    dispatch(stmt->cond());
    dispatch_loop_body(stmt->body());
}

void ScopeBuilder::visit_block_expr(NotNull<AstBlockExpr*> expr) {
    auto scope = register_scope(ScopeType::Block, expr);
    auto exit = enter_scope(scope);
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

SymbolId ScopeBuilder::register_decl(NotNull<AstNode*> node,
    InternedString name, const SymbolKey& key, const SymbolData& data) {
    TIRO_DEBUG_ASSERT(current_scope_, "Not inside a scope.");
    TIRO_DEBUG_ASSERT(
        node->id() == key.node(), "Symbol key and node must be consistent.");

    const auto scope_type = symbols_[current_scope_]->type();
    switch (data.type()) {
    case SymbolType::Import:
        TIRO_DEBUG_ASSERT(scope_type == ScopeType::File,
            "Imports are only allowed at file scope.");
        break;
    case SymbolType::TypeSymbol:
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
        Symbol(current_scope_, name, key, data));
    if (!sym_id) {
        node->has_error(true);
        diag_.reportf(Diagnostics::Error, node->source(),
            "The name '{}' has already been declared in this scope.",
            strings_.dump(name));
    }
    return sym_id;
}

ScopeId ScopeBuilder::register_scope(ScopeType type, NotNull<AstNode*> node) {
    TIRO_DEBUG_ASSERT(current_scope_, "Must have a current scope.");
    return symbols_.register_scope(
        current_scope_, current_func_, type, node->id());
}

ResetValue<ScopeId> ScopeBuilder::enter_scope(ScopeId new_scope) {
    auto old_scope = std::exchange(current_scope_, std::move(new_scope));
    return replace_value(current_scope_, std::move(old_scope));
}

ResetValue<SymbolId> ScopeBuilder::enter_func(SymbolId new_func) {
    auto old_func = std::exchange(current_func_, new_func);
    return replace_value(current_func_, std::move(old_func));
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
    auto scope = symbols_[scope_id];
    scope->is_loop_scope(true);
}

void ScopeBuilder::dispatch_children(NotNull<AstNode*> node) {
    node->traverse_children([&](AstNode* child) { dispatch(child); });
}

SymbolResolver::SymbolResolver(const SurroundingScopes& scopes,
    SymbolTable& table, const StringTable& strings, Diagnostics& diag)
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

void SymbolResolver::visit_func_decl(NotNull<AstFuncDecl*> func) {
    // Function names are visible from their bodies.
    activate(key(func));
    dispatch_children(func);
}

void SymbolResolver::visit_param_decl(NotNull<AstParamDecl*> param) {
    dispatch_children(param);
    activate(key(param));
}

void SymbolResolver::visit_var_decl(NotNull<AstVarDecl*> var) {
    struct ActivateVarVisitor {
        SymbolResolver& self;

        void visit_var_binding(NotNull<AstVarBinding*> v) {
            self.activate(key(v));
        }

        void visit_tuple_binding(NotNull<AstTupleBinding*> t) {
            const u32 name_count = checked_cast<u32>(t->names().size());
            for (u32 i = 0; i < name_count; ++i) {
                self.activate(key(t, i));
            }
        }
    };

    // Var is not active in initializer
    for (AstBinding* binding : var->bindings()) {
        if (!binding || binding->has_error())
            continue;

        dispatch(binding->init());
        visit(TIRO_NN(binding), ActivateVarVisitor{*this});
    }
}

void SymbolResolver::visit_decl([[maybe_unused]] NotNull<AstDecl*> decl) {
    // Must not be called. Special visit functions are needed for every subtype of AstDecl.
    TIRO_UNREACHABLE("Failed to overwrite decl type.");
}

void SymbolResolver::visit_file(NotNull<AstFile*> file) {
    // Function declarations in file scope are always active.
    auto scope = table_[table_.get_scope(file->id())];
    for (auto symbol_id : scope->entries()) {
        auto symbol = table_[symbol_id];

        // TODO: Variables / constants / classes visible?
        if (symbol->type() == SymbolType::Function) {
            symbol->active(true);
        }
    }

    dispatch_children(file);
}

void SymbolResolver::visit_var_expr(NotNull<AstVarExpr*> expr) {
    TIRO_CHECK(expr->name(), "Variable reference without a name.");

    auto expr_scope_id = scopes_.get(expr->id());
    auto [decl_scope_id, decl_symbol_id] = table_.find_name(
        expr_scope_id, expr->name());

    if (!decl_scope_id || !decl_symbol_id) {
        diag_.reportf(Diagnostics::Error, expr->source(),
            "Undefined symbol: '{}'.", strings_.value(expr->name()));
        expr->has_error(true);
        return;
    }

    auto expr_scope = table_[expr_scope_id];
    auto decl_scope = table_[decl_scope_id];
    auto decl_symbol = table_[decl_symbol_id];

    // Only symbols that are active by now can be referenced.
    if (!decl_symbol->active()) {
        diag_.reportf(Diagnostics::Error, expr->source(),
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

void SymbolResolver::activate(const SymbolKey& key) {
    auto symbol_id = table_.get_decl(key);
    auto symbol = table_[symbol_id];
    symbol->active(true);
}

void SymbolResolver::dispatch_children(NotNull<AstNode*> node) {
    node->traverse_children([&](AstNode* child) { dispatch(child); });
}

SymbolTable
resolve_symbols(AstNode* root, StringTable& strings, Diagnostics& diag) {
    SymbolTable table;
    SurroundingScopes scopes;

    // First pass: build scopes and register all declarations.
    {
        ScopeBuilder builder(scopes, table, strings, diag);
        builder.dispatch(root);
    }

    // Second pass: link references to symbol declarations.
    {
        SymbolResolver resolver(scopes, table, strings, diag);
        resolver.dispatch(root);
    }

    return table;
}

} // namespace tiro
