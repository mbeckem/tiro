#include "tiro/semantics/type_analyzer.hpp"

#include "tiro/compiler/diagnostics.hpp"
#include "tiro/semantics/analyzer.hpp"
#include "tiro/semantics/symbol_table.hpp"

namespace tiro {

TypeAnalyzer::TypeAnalyzer(Diagnostics& diag)
    : diag_(diag) {}

TypeAnalyzer::~TypeAnalyzer() {}

void TypeAnalyzer::dispatch(Node* node, bool required) {
    // TODO we might still be able to recurse into child nodes and check them,
    // even if the parent node contains errors?
    if (!node || node->has_error())
        return;

    visit(TIRO_NN(node), *this, required);
};

void TypeAnalyzer::visit_func_decl(
    FuncDecl* func, [[maybe_unused]] bool required) {
    dispatch(func->params(), false);
    dispatch(func->body(), false);
}

// A block used by other expressions must have an expression as its last statement
// and that expression must produce a value.
void TypeAnalyzer::visit_block_expr(BlockExpr* expr, bool required) {

    auto stmts = expr->stmts();
    const size_t stmt_count = stmts->size();
    if (stmt_count > 0) {
        for (size_t i = 0; i < stmt_count - 1; ++i) {
            const auto stmt = stmts->get(i);
            dispatch(stmt, false);
        }

        auto last_child = stmts->get(stmt_count - 1);
        dispatch(last_child, required);

        auto last_child_expr = try_cast<ExprStmt>(last_child);
        if (last_child_expr && can_use_as_value(last_child_expr->expr()))
            expr->expr_type(last_child_expr->expr()->expr_type());
    }

    if (required && !can_use_as_value(expr)) {
        if (stmt_count == 0) {
            diag_.report(Diagnostics::Error, expr->start(),
                "This block must produce a value: it cannot be empty.");
        } else {
            diag_.report(Diagnostics::Error, expr->start(),
                "This block must produce a value: the last statement must be a "
                "value-producing expression.");
        }

        // Act as if we had a value, even if we had an error above. Parent expressions can continue checking.
        expr->has_error(true);
        expr->expr_type(ExprType::Value);
    }
}

// If an if expr is used by other expressions, it must have two branches and both must produce a value.
void TypeAnalyzer::visit_if_expr(IfExpr* expr, bool required) {
    dispatch(expr->condition(), true);
    dispatch(expr->then_branch(), required);
    dispatch(expr->else_branch(), required);

    if (can_use_as_value(expr->then_branch()) && expr->else_branch()
        && can_use_as_value(expr->else_branch())) {
        const auto left = expr->then_branch()->expr_type();
        const auto right = expr->else_branch()->expr_type();
        expr->expr_type(left == ExprType::Value || right == ExprType::Value
                            ? ExprType::Value
                            : ExprType::Never);
    }

    if (required && !can_use_as_value(expr)) {
        if (!expr->else_branch()) {
            diag_.report(Diagnostics::Error, expr->start(),
                "This if expression must produce a value, the else branch must "
                "not be missing.");
        }

        // Act as if we had a value, even if we had an error above. Parent expressions can continue checking.
        expr->has_error(true);
        expr->expr_type(ExprType::Value);
    }
}

void TypeAnalyzer::visit_return_expr(
    ReturnExpr* expr, [[maybe_unused]] bool required) {
    dispatch(expr->inner(), true);
    expr->expr_type(ExprType::Never);
}

void TypeAnalyzer::visit_expr(Expr* expr, [[maybe_unused]] bool required) {
    visit_node(expr, required);

    const bool expr_returns = !(isa<ReturnExpr>(expr) || isa<ContinueExpr>(expr)
                                || isa<BreakExpr>(expr));
    expr->expr_type(expr_returns ? ExprType::Value : ExprType::Never);
}

void TypeAnalyzer::visit_assert_stmt(
    AssertStmt* stmt, [[maybe_unused]] bool required) {
    dispatch(stmt->condition(), true);
    dispatch(stmt->message(), true);
}

void TypeAnalyzer::visit_for_stmt(
    ForStmt* stmt, [[maybe_unused]] bool required) {
    dispatch(stmt->decl(), false);
    dispatch(stmt->condition(), true);
    dispatch(stmt->step(), false);
    dispatch(stmt->body(), false);
}

void TypeAnalyzer::visit_while_stmt(
    WhileStmt* stmt, [[maybe_unused]] bool required) {
    dispatch(stmt->condition(), true);
    dispatch(stmt->body(), false);
}

void TypeAnalyzer::visit_expr_stmt(ExprStmt* stmt, bool required) {
    dispatch(stmt->expr(), required);
}

void TypeAnalyzer::visit_binding(
    Binding* binding, [[maybe_unused]] bool required) {
    dispatch(binding->init(), true);
}

void TypeAnalyzer::visit_node(Node* node, [[maybe_unused]] bool required) {
    traverse_children(
        TIRO_NN(node), [&](Node* child) { dispatch(child, true); });
}

} // namespace tiro
