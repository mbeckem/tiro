#ifndef HAMMER_COMPILER_SEMANTICS_EXPR_ANALYZER_HPP
#define HAMMER_COMPILER_SEMANTICS_EXPR_ANALYZER_HPP

#include "hammer/compiler/syntax/ast.hpp"

namespace hammer::compiler {

/// Visits expressions and marks those we don't actually have to compute.
class ExprAnalyzer : public DefaultNodeVisitor<ExprAnalyzer, bool> {
public:
    ExprAnalyzer();
    ~ExprAnalyzer();

    ExprAnalyzer(const ExprAnalyzer&) = delete;
    ExprAnalyzer& operator=(const ExprAnalyzer&) = delete;

    void dispatch(const NodePtr<>& node, bool observed);

public:
    void visit_block_expr(
        const NodePtr<BlockExpr>& expr, bool observed) HAMMER_VISITOR_OVERRIDE;

    void visit_if_expr(
        const NodePtr<IfExpr>& expr, bool observed) HAMMER_VISITOR_OVERRIDE;

    void visit_expr(
        const NodePtr<Expr>& expr, bool observed) HAMMER_VISITOR_OVERRIDE;

    void visit_expr_stmt(
        const NodePtr<ExprStmt>& stmt, bool observed) HAMMER_VISITOR_OVERRIDE;

    void visit_for_stmt(
        const NodePtr<ForStmt>& stmt, bool observed) HAMMER_VISITOR_OVERRIDE;

    void visit_while_stmt(
        const NodePtr<WhileStmt>& stmt, bool observed) HAMMER_VISITOR_OVERRIDE;

    void
    visit_node(const NodePtr<>& node, bool observed) HAMMER_VISITOR_OVERRIDE;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_SEMANTICS_EXPR_ANALYZER_HPP
