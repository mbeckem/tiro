#include "hammer/compiler/semantics/expr_analyzer.hpp"

namespace hammer::compiler {

ExprAnalyzer::ExprAnalyzer() {}

ExprAnalyzer::~ExprAnalyzer() {}

void ExprAnalyzer::dispatch(const NodePtr<>& node, bool observed) {
    if (!node || node->has_error())
        return;

    visit(node, *this, observed);
}

void ExprAnalyzer::visit_block_expr(
    const NodePtr<BlockExpr>& expr, bool observed) {
    expr->observed(observed);

    auto stmts = expr->stmts();
    const size_t stmt_count = stmts->size();
    if (stmt_count > 0) {
        for (size_t i = 0; i < stmt_count - 1; ++i) {
            dispatch(stmts->get(i), false);
        }

        dispatch(stmts->get(stmt_count - 1), observed);
    }
}

void ExprAnalyzer::visit_if_expr(const NodePtr<IfExpr>& expr, bool observed) {
    expr->observed(observed);
    dispatch(expr->condition(), true);
    dispatch(expr->then_branch(), observed);
    dispatch(expr->else_branch(), observed);
}

void ExprAnalyzer::visit_expr(const NodePtr<Expr>& expr, bool observed) {
    expr->observed(observed);

    // Child expressions are observed by default
    visit_node(expr, true);
}

void ExprAnalyzer::visit_expr_stmt(
    const NodePtr<ExprStmt>& stmt, bool observed) {
    dispatch(stmt->expr(), observed);
}

void ExprAnalyzer::visit_node(
    const NodePtr<>& node, [[maybe_unused]] bool observed) {
    traverse_children(
        node, [&](const NodePtr<>& child) { dispatch(child, true); });
}

} // namespace hammer::compiler
