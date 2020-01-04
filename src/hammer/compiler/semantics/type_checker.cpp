#include "hammer/compiler/semantics/type_checker.hpp"

#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/semantics/analyzer.hpp"

namespace hammer::compiler {

TypeChecker::TypeChecker(Diagnostics& diag)
    : diag_(diag) {}

TypeChecker::~TypeChecker() {}

void TypeChecker::check(const NodePtr<>& node, bool requires_value) {
    // TODO we might still be able to recurse into child nodes and check them,
    // even if the parent node contains errors?

    if (!node || node->has_error())
        return;

    visit(node, *this, requires_value);
};

// A block used by other expressions must have an expression as its last statement
// and that expression must produce a value.
void TypeChecker::visit_block_expr(
    const NodePtr<BlockExpr>& expr, bool requires_value) {
    auto stmts = expr->stmts();
    const size_t stmt_count = stmts->size();
    if (stmt_count > 0) {
        for (size_t i = 0; i < stmt_count - 1; ++i) {
            check(stmts->get(i), false);
        }

        auto last_child = stmts->get(stmt_count - 1);
        if (requires_value && !isa<ExprStmt>(last_child)) {
            diag_.report(Diagnostics::Error, last_child->start(),
                "This block must produce a value: the last "
                "statement must be an expression.");
            expr->has_error(true);
        }

        check(last_child, requires_value);
        if (auto last_expr = try_cast<ExprStmt>(last_child);
            last_expr && can_use_as_value(last_expr->expr())) {
            expr->expr_type(last_expr->expr()->expr_type());
        }
    } else if (requires_value) {
        diag_.report(Diagnostics::Error, expr->start(),
            "This block must produce a value: it cannot be empty.");
        expr->has_error(true);
    }

    // Act as if we had a value, even if we had an error above. Parent expressions can continue checking.
    if (requires_value && !can_use_as_value(expr))
        expr->expr_type(ExprType::Value);
}

// If an if expr is used by other expressions, it must have two branches and both must produce a value.
void TypeChecker::visit_if_expr(
    const NodePtr<IfExpr>& expr, bool requires_value) {
    check(expr->condition(), true);
    check(expr->then_branch(), requires_value);

    if (requires_value && !expr->else_branch()) {
        diag_.report(Diagnostics::Error, expr->start(),
            "This if expression must produce a value: it must have "
            "an 'else' branch.");
        expr->has_error(true);
    }
    check(expr->else_branch(), requires_value);

    if (can_use_as_value(expr->then_branch()) && expr->else_branch()
        && can_use_as_value(expr->else_branch())) {
        const auto left = expr->then_branch()->expr_type();
        const auto right = expr->else_branch()->expr_type();
        expr->expr_type(left == ExprType::Value || right == ExprType::Value
                            ? ExprType::Value
                            : ExprType::Never);
    }

    // Act as if we had a value, even if we had an error above. Parent expressions can continue checking.
    if (requires_value && !can_use_as_value(expr))
        expr->expr_type(ExprType::Value);
}

void TypeChecker::visit_return_expr(
    const NodePtr<ReturnExpr>& expr, [[maybe_unused]] bool requires_value) {
    check(expr->inner(), true);
    expr->expr_type(ExprType::Never);
}

void TypeChecker::visit_expr(
    const NodePtr<Expr>& expr, [[maybe_unused]] bool requires_value) {
    traverse_children(
        expr, [&](const NodePtr<>& child) { check(child, true); });

    // FIXME: Optimization for useless dups, e.g. when assigning values.
    // The old way of doing things (below) does not work as written because
    // functions don't need to have a return value, but a function like this
    // should return the assignment value:
    //
    //      func(a, b) {
    //          a = b; // returns b
    //      }
    //
    // if (auto* binary = try_cast<BinaryExpr>(&expr);
    //    binary && binary->operation() == BinaryOperator::Assign) {
    //    binary->expr_type(
    //        requires_value ? ExprType::Value : ExprType::None);
    //    return;
    // }
    const bool expr_returns = !(isa<ReturnExpr>(expr) || isa<ContinueExpr>(expr)
                                || isa<BreakExpr>(expr));
    expr->expr_type(expr_returns ? ExprType::Value : ExprType::Never);
}

void TypeChecker::visit_assert_stmt(
    const NodePtr<AssertStmt>& stmt, [[maybe_unused]] bool requires_value) {
    check(stmt->condition(), true);
    check(stmt->message(), true);
}

void TypeChecker::visit_for_stmt(
    const NodePtr<ForStmt>& stmt, [[maybe_unused]] bool requires_value) {
    check(stmt->decl(), false);
    check(stmt->condition(), true);
    check(stmt->step(), false);
    check(stmt->body(), false);
}

void TypeChecker::visit_while_stmt(
    const NodePtr<WhileStmt>& stmt, [[maybe_unused]] bool requires_value) {
    check(stmt->condition(), true);
    check(stmt->body(), false);
}

void TypeChecker::visit_expr_stmt(
    const NodePtr<ExprStmt>& stmt, bool requires_value) {
    check(stmt->expr(), requires_value);
}

void TypeChecker::visit_var_decl(
    const NodePtr<VarDecl>& decl, [[maybe_unused]] bool requires_value) {
    check(decl->initializer(), true);
}

void TypeChecker::visit_node(
    const NodePtr<>& node, [[maybe_unused]] bool requires_value) {
    traverse_children(
        node, [&](const NodePtr<>& child) { check(child, false); });
}

} // namespace hammer::compiler
