#include "tiro/semantics/semantic_checker.hpp"

#include "tiro/compiler/diagnostics.hpp"
#include "tiro/core/string_table.hpp"
#include "tiro/semantics/analyzer.hpp"
#include "tiro/semantics/symbol_table.hpp"

namespace tiro {

SemanticChecker::SemanticChecker(
    SymbolTable& symbols, StringTable& strings, Diagnostics& diag)
    : symbols_(symbols)
    , strings_(strings)
    , diag_(diag) {}

SemanticChecker::~SemanticChecker() {}

void SemanticChecker::check(Node* node) {
    if (node && !node->has_error()) {
        visit(TIRO_NN(node), *this);
    }
}

void SemanticChecker::visit_root(Root* root) {
    TIRO_CHECK(root->file(), "Root does not have a file.");
    visit_node(root);
}

void SemanticChecker::visit_file(File* file) {
    const auto items = file->items();
    TIRO_CHECK(items && items->size() > 0, "File does not have any items.");

    for (Node* child : items->entries()) {
        if (!isa<FuncDecl>(child) && !isa<ImportDecl>(child)
            && !isa<DeclStmt>(child)) {
            // TODO: More items are allowed

            diag_.reportf(Diagnostics::Error, child->start(),
                "Invalid top level construct of type {}. Only "
                "functions, variables and imports are allowed for now.",
                to_string(child->type()));
            child->has_error(true);
            return;
        }
    }
    visit_node(file);
}

void SemanticChecker::visit_binding(Binding* binding) {
    const bool has_init = binding->init() != nullptr;

    visit_vars(TIRO_NN(binding), [&](VarDecl* var) {
        if (var->is_const() && !has_init) {
            diag_.reportf(Diagnostics::Error, binding->start(),
                "Constant is not being initialized.");
            var->has_error(true);
        }
    });
    visit_node(binding);
}

void SemanticChecker::visit_func_decl(FuncDecl* decl) {
    auto reset_func = enter_func(TIRO_NN(decl));
    visit_decl(decl);
}

void SemanticChecker::visit_for_stmt(ForStmt* stmt) {
    auto reset_loop = enter_loop(TIRO_NN(stmt));
    visit_ast_stmt(stmt);
}

void SemanticChecker::visit_while_stmt(WhileStmt* stmt) {
    auto reset_loop = enter_loop(TIRO_NN(stmt));
    visit_ast_stmt(stmt);
}

void SemanticChecker::visit_if_expr(IfExpr* expr) {
    if (const auto& e = expr->else_branch()) {
        TIRO_CHECK(isa<BlockExpr>(e) || isa<IfExpr>(e),
            "Invalid else branch of type {} (must be either a block or "
            "another if statement).");
    }
    visit_node(expr);
}

void SemanticChecker::visit_binary_expr(BinaryExpr* expr) {
    TIRO_CHECK(expr->left(), "Binary expression without a left child.");
    TIRO_CHECK(expr->right(), "Binary expression without a right child.");

    // Check the left hand side of all assignment operators.
    // Only certain expression kinds are allowed in this context.
    switch (expr->operation()) {
    case BinaryOperator::Assign:
    case BinaryOperator::AssignPlus:
    case BinaryOperator::AssignMinus:
    case BinaryOperator::AssignMultiply:
    case BinaryOperator::AssignDivide:
    case BinaryOperator::AssignModulus:
    case BinaryOperator::AssignPower: {
        const bool allow_tuple = expr->operation() == BinaryOperator::Assign;
        const auto lhs = expr->left();
        if (lhs->has_error() || !check_lhs_expr(expr->left(), allow_tuple))
            expr->has_error(true);
        break;
    }
    default:
        break;
    }

    visit_expr(expr);
}

void SemanticChecker::visit_continue_expr(ContinueExpr* expr) {
    if (!current_loop_) {
        diag_.reportf(Diagnostics::Error, expr->start(),
            "Continue expressions are not allowed outside a loop.");
        expr->has_error(true);
        return;
    }

    visit_expr(expr);
}

void SemanticChecker::visit_break_expr(BreakExpr* expr) {
    if (!current_loop_) {
        diag_.reportf(Diagnostics::Error, expr->start(),
            "Break expressions are not allowed outside a loop.");
        expr->has_error(true);
        return;
    }

    visit_expr(expr);
}

void SemanticChecker::visit_return_expr(ReturnExpr* expr) {
    if (!current_function_) {
        diag_.reportf(Diagnostics::Error, expr->start(),
            "Return expressions are not allowed outside a function.");
        expr->has_error(true);
        return;
    }

    visit_expr(expr);
}

void SemanticChecker::visit_node(Node* node) {
    traverse_children(TIRO_NN(node), [&](auto&& child) { check(child); });
}

bool SemanticChecker::check_lhs_expr(Expr* expr, bool allow_tuple) {
    TIRO_DEBUG_NOT_NULL(expr);

    if (isa<DotExpr>(expr) || isa<TupleMemberExpr>(expr)
        || isa<IndexExpr>(expr)) {
        return true;
    }

    if (VarExpr* lhs = try_cast<VarExpr>(expr)) {
        if (check_lhs_var(lhs)) {
            return true;
        }

        expr->has_error(true);
        return false;
    }

    if (TupleLiteral* lhs = try_cast<TupleLiteral>(expr)) {
        if (!allow_tuple) {
            diag_.report(Diagnostics::Error, expr->start(),
                "Tuple assignments are not supported in this context.");
            expr->has_error(true);
            return false;
        }

        const auto entries = lhs->entries();
        TIRO_DEBUG_NOT_NULL(entries);

        for (auto item : entries->entries()) {
            TIRO_DEBUG_NOT_NULL(item);

            if (!check_lhs_expr(item, false)) {
                expr->has_error(true);
                return false;
            }
        }

        return true;
    }

    diag_.reportf(Diagnostics::Error, expr->start(),
        "Cannot use operand of type {} as the left hand side of an "
        "assignment.",
        to_string(expr->type()));
    expr->has_error(true);
    return false;
}

bool SemanticChecker::check_lhs_var(VarExpr* expr) {
    TIRO_DEBUG_NOT_NULL(expr);

    auto entry = expr->resolved_symbol();
    TIRO_DEBUG_NOT_NULL(entry);

    auto decl = entry->decl();
    TIRO_DEBUG_NOT_NULL(decl);

    struct AssignmentChecker {
        VarExpr* expr_;
        SemanticChecker& checker_;

        AssignmentChecker(VarExpr* expr, SemanticChecker& checker)
            : expr_(expr)
            , checker_(checker) {}

        bool visit_var_decl(VarDecl* decl) {
            if (decl->is_const()) {
                checker_.diag_.reportf(Diagnostics::Error, expr_->start(),
                    "Cannot assign to the constant '{}'.",
                    checker_.strings_.value(decl->name()));
                expr_->has_error(true);
                return false;
            }
            return true;
        }

        bool visit_param_decl(ParamDecl*) { return true; }

        bool visit_func_decl(FuncDecl* decl) {
            checker_.diag_.reportf(Diagnostics::Error, expr_->start(),
                "Cannot assign to the function '{}'.",
                checker_.strings_.value(decl->name()));
            expr_->has_error(true);
            return false;
        }

        bool visit_import_decl(ImportDecl* decl) {
            checker_.diag_.reportf(Diagnostics::Error, expr_->start(),
                "Cannot assign to the imported symbol '{}'.",
                checker_.strings_.value(decl->name()));
            expr_->has_error(true);
            return false;
        }
    } check_assign(expr, *this);

    return visit(TIRO_NN(decl), check_assign);
}

ResetValue<Ref<Node>> SemanticChecker::enter_loop(NotNull<Node*> loop) {
    return replace_value(current_loop_, ref(loop.get()));
}

ResetValue<Ref<Node>> SemanticChecker::enter_func(NotNull<Node*> func) {
    return replace_value(current_function_, ref(func.get()));
}

} // namespace tiro
