#ifndef TIRO_SEMANTICS_SEMANTIC_CHECKER_HPP
#define TIRO_SEMANTICS_SEMANTIC_CHECKER_HPP

#include "tiro/compiler/reset_value.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/syntax/ast.hpp"

namespace tiro {

class StringTable;

} // namespace tiro

namespace tiro {

class Diagnostics;
class SymbolTable;

class SemanticChecker final : public DefaultNodeVisitor<SemanticChecker> {
public:
    explicit SemanticChecker(
        SymbolTable& symbols, StringTable& strings, Diagnostics& diag);
    ~SemanticChecker();

    SemanticChecker(const SemanticChecker&) = delete;
    SemanticChecker& operator=(const SemanticChecker&) = delete;

    void check(Node* node);

    void visit_root(Root* root) TIRO_VISITOR_OVERRIDE;
    void visit_file(File* file) TIRO_VISITOR_OVERRIDE;
    void visit_binding(Binding* binding) TIRO_VISITOR_OVERRIDE;

    void visit_func_decl(FuncDecl* decl) TIRO_VISITOR_OVERRIDE;
    void visit_for_stmt(ForStmt* stmt) TIRO_VISITOR_OVERRIDE;
    void visit_while_stmt(WhileStmt* stmt) TIRO_VISITOR_OVERRIDE;

    void visit_if_expr(IfExpr* expr) TIRO_VISITOR_OVERRIDE;
    void visit_binary_expr(BinaryExpr* expr) TIRO_VISITOR_OVERRIDE;
    void visit_continue_expr(ContinueExpr* expr) TIRO_VISITOR_OVERRIDE;
    void visit_break_expr(BreakExpr* expr) TIRO_VISITOR_OVERRIDE;
    void visit_return_expr(ReturnExpr* expr) TIRO_VISITOR_OVERRIDE;
    void visit_node(Node* node) TIRO_VISITOR_OVERRIDE;

private:
    bool check_lhs_expr(Expr* expr, bool allow_tuple);
    bool check_lhs_var(VarExpr* expr);

    ResetValue<Ref<Node>> enter_loop(NotNull<Node*> loop);
    ResetValue<Ref<Node>> enter_func(NotNull<Node*> func);

private:
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;

    Ref<Node> current_function_;
    Ref<Node> current_loop_;
};

} // namespace tiro

#endif // TIRO_SEMANTICS_SEMANTIC_CHECKER_HPP
