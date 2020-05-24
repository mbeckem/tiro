#ifndef TIRO_SEMANTICS_SCOPE_BUILDER_HPP
#define TIRO_SEMANTICS_SCOPE_BUILDER_HPP

#include "tiro/ast/ast.hpp"
#include "tiro/compiler/reset_value.hpp"
#include "tiro/semantics/fwd.hpp"

namespace tiro {

class StringTable;

} // namespace tiro

namespace tiro {

class SymbolTable;
class Diagnostics;

/// The scope builder assembles the tree of nested scopes.
class ScopeBuilder final : public DefaultNodeVisitor<ScopeBuilder> {
public:
    explicit ScopeBuilder(
        SymbolTable& symbols, StringTable& strings, Diagnostics& diag);
    ~ScopeBuilder();

    ScopeBuilder(const ScopeBuilder&) = delete;
    ScopeBuilder& operator=(const ScopeBuilder&) = delete;

    void dispatch(AstNode* node);

    void visit_file(NotNull<AstFile*> file) TIRO_NODE_VISITOR_OVERRIDE;

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
    SymbolId register_decl(NotNull<AstNode*> node, SymbolType type,
        InternedString name, const SymbolKey& key);

    ScopeId register_scope(ScopeType type, NotNull<AstNode*> node);

    ResetValue<ScopeId> enter_scope(ScopeId new_scope);
    ResetValue<AstFuncDecl*> enter_func(NotNull<AstFuncDecl*> new_func);

    void dispatch_block(AstExpr* node);

    void dispatch_children(NotNull<AstNode*> node);

private:
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;

    ScopeId global_scope_;
    ScopeId current_scope_;
    AstFuncDecl* current_func_ = nullptr;

    // Maps the ast id (a symbol reference) to the surrounding scope.
    // Symbols are resolved after all declarations have been processed.
    // TODO: Better hash map container
    std::unordered_map<AstId, ScopeId, UseHasher> surrounding_scope_;
};

} // namespace tiro

#endif // TIRO_SEMANTICS_SCOPE_BUILDER_HPP
