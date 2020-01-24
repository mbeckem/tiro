#ifndef TIRO_SEMANTICS_SEMANTIC_CHECKER_HPP
#define TIRO_SEMANTICS_SEMANTIC_CHECKER_HPP

#include "tiro/syntax/ast.hpp"

namespace tiro::compiler {

class Diagnostics;
class StringTable;
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
    void visit_if_expr(IfExpr* expr) TIRO_VISITOR_OVERRIDE;
    void visit_binary_expr(BinaryExpr* expr) TIRO_VISITOR_OVERRIDE;
    void visit_node(Node* node) TIRO_VISITOR_OVERRIDE;

private:
    bool check_lhs_expr(Expr* expr, bool allow_tuple);
    bool check_lhs_var(VarExpr* expr);

private:
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace tiro::compiler

#endif // TIRO_SEMANTICS_SEMANTIC_CHECKER_HPP
