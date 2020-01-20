#ifndef TIRO_SEMANTICS_EXPR_ANALYZER_HPP
#define TIRO_SEMANTICS_EXPR_ANALYZER_HPP

#include "tiro/syntax/ast.hpp"

namespace tiro::compiler {

/// Visits expressions and marks those we don't actually have to compute.
class ExprAnalyzer : public DefaultNodeVisitor<ExprAnalyzer, bool> {
public:
    ExprAnalyzer();
    ~ExprAnalyzer();

    ExprAnalyzer(const ExprAnalyzer&) = delete;
    ExprAnalyzer& operator=(const ExprAnalyzer&) = delete;

    void dispatch(Node* node, bool observed);

public:
    void visit_block_expr(BlockExpr* expr, bool observed) TIRO_VISITOR_OVERRIDE;

    void visit_if_expr(IfExpr* expr, bool observed) TIRO_VISITOR_OVERRIDE;

    void visit_expr(Expr* expr, bool observed) TIRO_VISITOR_OVERRIDE;

    void visit_expr_stmt(ExprStmt* stmt, bool observed) TIRO_VISITOR_OVERRIDE;

    void visit_for_stmt(ForStmt* stmt, bool observed) TIRO_VISITOR_OVERRIDE;

    void visit_while_stmt(WhileStmt* stmt, bool observed) TIRO_VISITOR_OVERRIDE;

    void visit_node(Node* node, bool observed) TIRO_VISITOR_OVERRIDE;
};

} // namespace tiro::compiler

#endif // TIRO_SEMANTICS_EXPR_ANALYZER_HPP
