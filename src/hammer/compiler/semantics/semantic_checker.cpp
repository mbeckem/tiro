#include "hammer/compiler/semantics/semantic_checker.hpp"

#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/semantics/analyzer.hpp"
#include "hammer/compiler/semantics/symbol_table.hpp"
#include "hammer/compiler/string_table.hpp"

namespace hammer::compiler {

SemanticChecker::SemanticChecker(
    SymbolTable& symbols, StringTable& strings, Diagnostics& diag)
    : symbols_(symbols)
    , strings_(strings)
    , diag_(diag) {}

SemanticChecker::~SemanticChecker() {}

void SemanticChecker::check(Node* node) {
    if (node && !node->has_error()) {
        visit(node, *this);
    }
}

void SemanticChecker::visit_root(Root* root) {
    HAMMER_CHECK(root->file(), "Root does not have a file.");
    visit_node(root);
}

void SemanticChecker::visit_file(File* file) {
    const auto items = file->items();
    HAMMER_CHECK(items && items->size() > 0, "File does not have any items.");

    for (Node* child : items->entries()) {
        if (!isa<FuncDecl>(child) && !isa<ImportDecl>(child)) {
            // TODO: More items are allowed

            diag_.reportf(Diagnostics::Error, child->start(),
                "Invalid top level construct of type {}. Only "
                "functions and imports are allowed for now.",
                to_string(child->type()));
            child->has_error(true);
            return;
        }
    }
    visit_node(file);
}

void SemanticChecker::visit_binding(Binding* binding) {
    const bool has_init = binding->init() != nullptr;

    visit_vars(binding, [&](VarDecl* var) {
        if (var->is_const() && !has_init) {
            diag_.reportf(Diagnostics::Error, binding->start(),
                "Constant is not being initialized.");
            var->has_error(true);
        }
    });
    visit_node(binding);
}

void SemanticChecker::visit_if_expr(IfExpr* expr) {
    if (const auto& e = expr->else_branch()) {
        HAMMER_CHECK(isa<BlockExpr>(e) || isa<IfExpr>(e),
            "Invalid else branch of type {} (must be either a block or "
            "another if statement).");
    }
    visit_node(expr);
}

void SemanticChecker::visit_binary_expr(BinaryExpr* expr) {
    HAMMER_CHECK(expr->left(), "Binary expression without a left child.");
    HAMMER_CHECK(expr->right(), "Binary expression without a right child.");

    if (expr->operation() == BinaryOperator::Assign) {
        auto lhs = expr->left();
        if (lhs->has_error() || !check_lhs_expr(expr->left(), true))
            expr->has_error(true);
    }
    visit_expr(expr);
}

void SemanticChecker::visit_node(Node* node) {
    traverse_children(node, [&](auto&& child) { check(child); });
}

bool SemanticChecker::check_lhs_expr(Expr* expr, bool allow_tuple) {
    HAMMER_ASSERT_NOT_NULL(expr);

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
            // TODO
            diag_.report(Diagnostics::Error, expr->start(),
                "Nested tuple assignments are not supported.");
            expr->has_error(true);
            return false;
        }

        const auto entries = lhs->entries();
        HAMMER_ASSERT_NOT_NULL(entries);

        for (auto item : entries->entries()) {
            HAMMER_ASSERT_NOT_NULL(item);

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
    HAMMER_ASSERT_NOT_NULL(expr);

    auto entry = expr->resolved_symbol();
    HAMMER_ASSERT_NOT_NULL(entry);

    auto decl = entry->decl();
    HAMMER_ASSERT_NOT_NULL(decl);

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

    return visit(decl, check_assign);
}

} // namespace hammer::compiler
