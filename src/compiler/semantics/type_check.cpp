#include "compiler/semantics/type_check.hpp"

#include "compiler/ast/ast.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/semantics/analysis.hpp"
#include "compiler/semantics/symbol_table.hpp"

namespace tiro {

namespace {

/// The recursive tree walk assigns a value type other than None everywhere an actual value
/// is generated. When a value is "required" (e.g. part of an expression) then it *MUST*
/// produce an actual value.
class TypeAnalyzer final : public DefaultNodeVisitor<TypeAnalyzer, bool> {
public:
    explicit TypeAnalyzer(TypeTable& types, Diagnostics& diag);
    ~TypeAnalyzer();

    TypeAnalyzer(const TypeAnalyzer&) = delete;
    TypeAnalyzer& operator=(const TypeAnalyzer&) = delete;

    void dispatch(AstNode* node, bool required);

    void visit_func_decl(NotNull<AstFuncDecl*> func, bool required) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_block_expr(NotNull<AstBlockExpr*> expr, bool required) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_if_expr(NotNull<AstIfExpr*> expr, bool required) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_return_expr(NotNull<AstReturnExpr*> expr, bool required) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_expr(NotNull<AstExpr*> expr, bool required) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_assert_stmt(NotNull<AstAssertStmt*> stmt, bool required) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_defer_stmt(NotNull<AstDeferStmt*> stmt, bool required) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_while_stmt(NotNull<AstWhileStmt*> stmt, bool required) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_for_stmt(NotNull<AstForStmt*> stmt, bool required) TIRO_NODE_VISITOR_OVERRIDE;

    void
    visit_for_each_stmt(NotNull<AstForEachStmt*> stmt, bool required) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_expr_stmt(NotNull<AstExprStmt*> stmt, bool required) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_binding(NotNull<AstBinding*> binding, bool required) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_node(NotNull<AstNode*> node, bool required) TIRO_NODE_VISITOR_OVERRIDE;

private:
    void register_type(NotNull<AstExpr*> expr, ExprType type);

    ExprType get_type(NotNull<AstExpr*> expr);

    template<typename T>
    void dispatch_list(const AstNodeList<T>& list, bool required) {
        for (auto node : list)
            dispatch(node, required);
    }

private:
    TypeTable& types_;
    Diagnostics& diag_;
};

} // namespace

TypeAnalyzer::TypeAnalyzer(TypeTable& types, Diagnostics& diag)
    : types_(types)
    , diag_(diag) {}

TypeAnalyzer::~TypeAnalyzer() {}

void TypeAnalyzer::dispatch(AstNode* node, bool required) {
    // TODO we might still be able to recurse into child nodes and check them,
    // even if the parent node contains errors?
    if (!node || node->has_error())
        return;

    visit(TIRO_NN(node), *this, required);
};

void TypeAnalyzer::visit_func_decl(NotNull<AstFuncDecl*> func, [[maybe_unused]] bool required) {
    dispatch_list(func->params(), false);
    dispatch(func->body(), func->body_is_value());
}

// A block used by other expressions must have an expression as its last statement
// and that expression must produce a value.
void TypeAnalyzer::visit_block_expr(NotNull<AstBlockExpr*> expr, bool required) {
    ExprType type = ExprType::None;

    const auto& stmts = expr->stmts();
    const size_t stmt_count = stmts.size();
    if (stmt_count > 0) {
        for (size_t i = 0; i < stmt_count - 1; ++i)
            dispatch(stmts.get(i), false);

        auto last_child = stmts.get(stmt_count - 1);
        dispatch(last_child, required);

        if (auto expr_stmt = try_cast<AstExprStmt>(last_child)) {
            auto expr_type = get_type(TIRO_NN(expr_stmt->expr()));
            if (can_use_as_value(expr_type))
                type = expr_type;
        }
    }

    if (required && !can_use_as_value(type)) {
        if (stmt_count == 0) {
            diag_.report(Diagnostics::Error, expr->source(),
                "This block must produce a value: it cannot be empty.");
        } else {
            diag_.report(Diagnostics::Error, expr->source(),
                "This block must produce a value: the last statement must be a "
                "value-producing expression.");
        }

        // Act as if we had a value, even if we had an error above. Parent expressions can continue checking.
        expr->has_error(true);
        type = ExprType::Value;
    }

    register_type(expr, type);
}

// If an if expr is used by other expressions, it must have two branches and both must produce a value.
void TypeAnalyzer::visit_if_expr(NotNull<AstIfExpr*> expr, bool required) {
    dispatch(expr->cond(), true);
    dispatch(expr->then_branch(), required);
    dispatch(expr->else_branch(), required);

    ExprType type = ExprType::None;

    if (expr->then_branch() && expr->else_branch()) {
        ExprType then_type = get_type(TIRO_NN(expr->then_branch()));
        ExprType else_type = get_type(TIRO_NN(expr->else_branch()));

        if (can_use_as_value(then_type) && can_use_as_value(else_type)) {
            type = (then_type == ExprType::Value || else_type == ExprType::Value) ? ExprType::Value
                                                                                  : ExprType::Never;
        }
    }

    if (required && !can_use_as_value(type)) {
        if (!expr->else_branch()) {
            diag_.report(Diagnostics::Error, expr->source(),
                "This if expression must produce a value, the else branch must "
                "not be missing.");
        }

        // Act as if we had a value, even if we had an error above. Parent expressions can continue checking.
        expr->has_error(true);
        type = ExprType::Value;
    }

    register_type(expr, type);
}

void TypeAnalyzer::visit_return_expr(NotNull<AstReturnExpr*> expr, [[maybe_unused]] bool required) {
    dispatch(expr->value(), true);
    register_type(expr, ExprType::Never);
}

void TypeAnalyzer::visit_expr(NotNull<AstExpr*> expr, [[maybe_unused]] bool required) {
    visit_node(expr, required);

    // Every value not handled by one of the special visitor functions produces a value by
    // default.
    if (is_instance<AstContinueExpr>(expr) || is_instance<AstBreakExpr>(expr)) {
        register_type(expr, ExprType::Never);
    } else {
        register_type(expr, ExprType::Value);
    }
}

void TypeAnalyzer::visit_assert_stmt(NotNull<AstAssertStmt*> stmt, [[maybe_unused]] bool required) {
    dispatch(stmt->cond(), true);
    dispatch(stmt->message(), true);
}

void TypeAnalyzer::visit_for_stmt(NotNull<AstForStmt*> stmt, [[maybe_unused]] bool required) {
    dispatch(stmt->decl(), false);
    dispatch(stmt->cond(), true);
    dispatch(stmt->step(), false);
    dispatch(stmt->body(), false);
}

void TypeAnalyzer::visit_for_each_stmt(
    NotNull<AstForEachStmt*> stmt, [[maybe_unused]] bool required) {
    dispatch(stmt->spec(), false);
    dispatch(stmt->expr(), true);
    dispatch(stmt->body(), false);
}

void TypeAnalyzer::visit_defer_stmt(NotNull<AstDeferStmt*> stmt, [[maybe_unused]] bool required) {
    dispatch(stmt->expr(), false);
}

void TypeAnalyzer::visit_while_stmt(NotNull<AstWhileStmt*> stmt, [[maybe_unused]] bool required) {
    dispatch(stmt->cond(), true);
    dispatch(stmt->body(), false);
}

void TypeAnalyzer::visit_expr_stmt(NotNull<AstExprStmt*> stmt, bool required) {
    dispatch(stmt->expr(), required);
}

void TypeAnalyzer::visit_binding(NotNull<AstBinding*> binding, [[maybe_unused]] bool required) {
    dispatch(binding->init(), true);
}

void TypeAnalyzer::visit_node(NotNull<AstNode*> node, [[maybe_unused]] bool required) {
    node->traverse_children([&](AstNode* child) { dispatch(child, true); });
}

void TypeAnalyzer::register_type(NotNull<AstExpr*> expr, ExprType type) {
    return types_.register_type(expr->id(), type);
}

ExprType TypeAnalyzer::get_type(NotNull<AstExpr*> expr) {
    return types_.get_type(expr->id());
}

void check_types(NotNull<AstNode*> node, TypeTable& types, Diagnostics& diag) {
    TypeAnalyzer analyzer(types, diag);
    analyzer.dispatch(node, false);
}

} // namespace tiro
