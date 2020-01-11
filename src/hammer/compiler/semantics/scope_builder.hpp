#ifndef HAMMER_COMPILER_SEMANTICS_SCOPE_BUILDER_HPP
#define HAMMER_COMPILER_SEMANTICS_SCOPE_BUILDER_HPP

#include "hammer/compiler/reset_value.hpp"
#include "hammer/compiler/semantics/symbol_table.hpp"
#include "hammer/compiler/syntax/ast.hpp"

namespace hammer::compiler {

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

    void visit_root(Root* root) HAMMER_VISITOR_OVERRIDE;
    void visit_file(File* file) HAMMER_VISITOR_OVERRIDE;
    void visit_func_decl(FuncDecl* func) HAMMER_VISITOR_OVERRIDE;
    void visit_decl(Decl* decl) HAMMER_VISITOR_OVERRIDE;
    void visit_for_stmt(ForStmt* stmt) HAMMER_VISITOR_OVERRIDE;
    void visit_while_stmt(WhileStmt* stmt) HAMMER_VISITOR_OVERRIDE;
    void visit_block_expr(BlockExpr* expr) HAMMER_VISITOR_OVERRIDE;
    void visit_var_expr(VarExpr* expr) HAMMER_VISITOR_OVERRIDE;
    void visit_node(Node* node) HAMMER_VISITOR_OVERRIDE;

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

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_SEMANTICS_SCOPE_BUILDER_HPP
