#include "tiro/semantics/expr_analyzer.hpp"

#include "tiro/semantics/analyzer.hpp"
#include "tiro/semantics/symbol_table.hpp"

namespace tiro {

ExprAnalyzer::ExprAnalyzer() {}

ExprAnalyzer::~ExprAnalyzer() {}

void ExprAnalyzer::dispatch(Node* node, bool observed) {
    if (!node || node->has_error())
        return;

    visit(TIRO_NN(node), *this, observed);
}

void ExprAnalyzer::visit_block_expr(BlockExpr* expr, bool observed) {
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

void ExprAnalyzer::visit_if_expr(IfExpr* expr, bool observed) {
    expr->observed(observed);

    const bool arms_observed = observed && can_use_as_value(expr);
    dispatch(expr->condition(), true);
    dispatch(expr->then_branch(), arms_observed);
    dispatch(expr->else_branch(), arms_observed);
}

void ExprAnalyzer::visit_expr(Expr* expr, bool observed) {
    expr->observed(observed);

    // Child expressions are observed by default
    visit_node(expr, true);
}

void ExprAnalyzer::visit_expr_stmt(ExprStmt* stmt, bool observed) {
    dispatch(stmt->expr(), observed);
}

void ExprAnalyzer::visit_for_stmt(
    ForStmt* stmt, [[maybe_unused]] bool observed) {
    dispatch(stmt->decl(), false);
    dispatch(stmt->condition(), true);
    dispatch(stmt->step(), false);
    dispatch(stmt->body(), false);
}

void ExprAnalyzer::visit_while_stmt(
    WhileStmt* stmt, [[maybe_unused]] bool observed) {
    dispatch(stmt->condition(), true);
    dispatch(stmt->body(), false);
}

void ExprAnalyzer::visit_node(Node* node, [[maybe_unused]] bool observed) {
    traverse_children(
        TIRO_NN(node), [&](Node* child) { dispatch(child, true); });
}

} // namespace tiro
