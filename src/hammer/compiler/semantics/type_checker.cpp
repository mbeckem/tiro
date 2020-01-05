#include "hammer/compiler/semantics/type_checker.hpp"

#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/semantics/analyzer.hpp"

namespace hammer::compiler {

// TODO: It would be more maintainable to extract the detection of unobserved
// expressions out of this class.

TypeChecker::TypeChecker(Diagnostics& diag)
    : diag_(diag) {}

TypeChecker::~TypeChecker() {}

void TypeChecker::check(const NodePtr<>& node, TypeRequirement req) {
    // TODO we might still be able to recurse into child nodes and check them,
    // even if the parent node contains errors?

    if (!node || node->has_error())
        return;

    visit(node, *this, req);
};

void TypeChecker::visit_func_decl(
    const NodePtr<FuncDecl>& func, [[maybe_unused]] TypeRequirement req) {
    check(func->params(), TypeRequirement::IGNORE);
    check(func->body(), TypeRequirement::PREFER_VALUE);
}

// A block used by other expressions must have an expression as its last statement
// and that expression must produce a value.
void TypeChecker::visit_block_expr(
    const NodePtr<BlockExpr>& expr, TypeRequirement req) {
    auto stmts = expr->stmts();
    const size_t stmt_count = stmts->size();
    if (stmt_count > 0) {
        for (size_t i = 0; i < stmt_count - 1; ++i) {
            const auto stmt = stmts->get(i);
            check(stmt, TypeRequirement::IGNORE);
        }

        auto last_child = stmts->get(stmt_count - 1);
        if (req == TypeRequirement::REQUIRE_VALUE
            && !isa<ExprStmt>(last_child)) {
            diag_.report(Diagnostics::Error, last_child->start(),
                "This block has to produce a value: the last "
                "statement must be an expression.");
            expr->has_error(true);
        }

        check(last_child, req);
        if (req != TypeRequirement::IGNORE) {
            if (auto last_expr = try_cast<ExprStmt>(last_child);
                last_expr && can_use_as_value(last_expr->expr())) {
                expr->expr_type(last_expr->expr()->expr_type());
            }
        }
    } else if (req == TypeRequirement::REQUIRE_VALUE) {
        diag_.report(Diagnostics::Error, expr->start(),
            "This block must produce a value: it cannot be empty.");
        expr->has_error(true);
    }

    // Act as if we had a value, even if we had an error above. Parent expressions can continue checking.
    if (req == TypeRequirement::REQUIRE_VALUE && !can_use_as_value(expr))
        expr->expr_type(ExprType::Value);
}

// If an if expr is used by other expressions, it must have two branches and both must produce a value.
void TypeChecker::visit_if_expr(
    const NodePtr<IfExpr>& expr, TypeRequirement req) {
    check(expr->condition(), TypeRequirement::REQUIRE_VALUE);
    check(expr->then_branch(), req);

    if (req == TypeRequirement::REQUIRE_VALUE && !expr->else_branch()) {
        diag_.report(Diagnostics::Error, expr->start(),
            "This if expression must produce a value: it must have "
            "an 'else' branch.");
        expr->has_error(true);
    }
    check(expr->else_branch(), req);

    if (can_use_as_value(expr->then_branch()) && expr->else_branch()
        && can_use_as_value(expr->else_branch())) {
        const auto left = expr->then_branch()->expr_type();
        const auto right = expr->else_branch()->expr_type();
        expr->expr_type(left == ExprType::Value || right == ExprType::Value
                            ? ExprType::Value
                            : ExprType::Never);
    }

    // Act as if we had a value, even if we had an error above. Parent expressions can continue checking.
    if (req == TypeRequirement::REQUIRE_VALUE && !can_use_as_value(expr))
        expr->expr_type(ExprType::Value);
}

void TypeChecker::visit_return_expr(
    const NodePtr<ReturnExpr>& expr, [[maybe_unused]] TypeRequirement req) {
    check(expr->inner(), TypeRequirement::REQUIRE_VALUE);
    expr->expr_type(ExprType::Never);
}

void TypeChecker::visit_expr(const NodePtr<Expr>& expr, TypeRequirement req) {
    if (req == TypeRequirement::IGNORE)
        expr->observed(false);

    traverse_children(expr, [&](const NodePtr<>& child) {
        // TODO: Would be nice to support "ignore" for nested expresions too.
        // This would require to make the codegen of expressions (e.g. math ops) aware
        // of the fact that they are not needed.
        check(child, TypeRequirement::REQUIRE_VALUE);
    });

    const bool expr_returns = !(isa<ReturnExpr>(expr) || isa<ContinueExpr>(expr)
                                || isa<BreakExpr>(expr));
    expr->expr_type(expr_returns ? ExprType::Value : ExprType::Never);
}

void TypeChecker::visit_assert_stmt(
    const NodePtr<AssertStmt>& stmt, [[maybe_unused]] TypeRequirement req) {
    check(stmt->condition(), TypeRequirement::REQUIRE_VALUE);
    check(stmt->message(), TypeRequirement::REQUIRE_VALUE);
}

void TypeChecker::visit_for_stmt(
    const NodePtr<ForStmt>& stmt, [[maybe_unused]] TypeRequirement req) {
    check(stmt->decl(), TypeRequirement::IGNORE);
    check(stmt->condition(), TypeRequirement::REQUIRE_VALUE);
    check(stmt->step(), TypeRequirement::IGNORE);
    check(stmt->body(), TypeRequirement::IGNORE);
}

void TypeChecker::visit_while_stmt(
    const NodePtr<WhileStmt>& stmt, [[maybe_unused]] TypeRequirement req) {
    check(stmt->condition(), TypeRequirement::REQUIRE_VALUE);
    check(stmt->body(), TypeRequirement::IGNORE);
}

void TypeChecker::visit_expr_stmt(
    const NodePtr<ExprStmt>& stmt, TypeRequirement req) {
    check(stmt->expr(), req);
}

void TypeChecker::visit_binding(
    const NodePtr<Binding>& binding, [[maybe_unused]] TypeRequirement req) {
    check(binding->init(), TypeRequirement::REQUIRE_VALUE);
}

void TypeChecker::visit_node(
    const NodePtr<>& node, [[maybe_unused]] TypeRequirement req) {
    traverse_children(node, [&](const NodePtr<>& child) { check(child, req); });
}

} // namespace hammer::compiler
