#ifndef TIRO_COMPILER_SEMANTICS_SCOPE_BUILDER_HPP
#define TIRO_COMPILER_SEMANTICS_SCOPE_BUILDER_HPP

#include "tiro/compiler/reset_value.hpp"
#include "tiro/compiler/semantics/symbol_table.hpp"
#include "tiro/compiler/syntax/ast.hpp"

namespace tiro::compiler {

class StringTable;
class SymbolTable;
class Diagnostics;

/// The scope builder assembles the tree of nested scopes.
/// Every declaration receives a symbol entry in its containing scope.
/// Variables are not being resolved yet (that is done in a second pass).
class ScopeBuilder final : public DefaultNodeVisitor<ScopeBuilder> {
public:
    explicit ScopeBuilder(SymbolTable& symbols, StringTable& strings,
        Diagnostics& diag, ScopePtr global_scope);
    ~ScopeBuilder();

    ScopeBuilder(const ScopeBuilder&) = delete;
    ScopeBuilder& operator=(const ScopeBuilder&) = delete;

    void dispatch(Node* node);

    void visit_root(Root* root) TIRO_VISITOR_OVERRIDE;
    void visit_file(File* file) TIRO_VISITOR_OVERRIDE;
    void visit_func_decl(FuncDecl* func) TIRO_VISITOR_OVERRIDE;
    void visit_decl(Decl* decl) TIRO_VISITOR_OVERRIDE;
    void visit_for_stmt(ForStmt* stmt) TIRO_VISITOR_OVERRIDE;
    void visit_while_stmt(WhileStmt* stmt) TIRO_VISITOR_OVERRIDE;
    void visit_block_expr(BlockExpr* expr) TIRO_VISITOR_OVERRIDE;
    void visit_var_expr(VarExpr* expr) TIRO_VISITOR_OVERRIDE;
    void visit_node(Node* node) TIRO_VISITOR_OVERRIDE;

private:
    void add_decl(Decl* decl);

    ResetValue<ScopePtr> enter_scope(ScopePtr new_scope);

    ResetValue<NodePtr<FuncDecl>> enter_func(FuncDecl* new_func);

    void dispatch_children(Node* node);

private:
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;

    ScopePtr global_scope_;
    ScopePtr current_scope_;
    NodePtr<FuncDecl> current_func_;
};

} // namespace tiro::compiler

#endif // TIRO_COMPILER_SEMANTICS_SCOPE_BUILDER_HPP
