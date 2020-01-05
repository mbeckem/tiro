#include "hammer/compiler/semantics/semantic_checker.hpp"

#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/semantics/symbol_table.hpp"
#include "hammer/compiler/string_table.hpp"

namespace hammer::compiler {

SemanticChecker::SemanticChecker(
    SymbolTable& symbols, StringTable& strings, Diagnostics& diag)
    : symbols_(symbols)
    , strings_(strings)
    , diag_(diag) {}

SemanticChecker::~SemanticChecker() {}

void SemanticChecker::check(const NodePtr<>& node) {
    if (node && !node->has_error()) {
        visit(node, *this);
    }
}

void SemanticChecker::visit_root(const NodePtr<Root>& root) {
    HAMMER_CHECK(root->file(), "Root does not have a file.");
    visit_node(root);
}

void SemanticChecker::visit_file(const NodePtr<File>& file) {
    const auto items = file->items();
    HAMMER_CHECK(items && items->size() > 0, "File does not have any items.");

    for (const NodePtr<> child : items->entries()) {
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

void SemanticChecker::visit_if_expr(const NodePtr<IfExpr>& expr) {
    if (const auto& e = expr->else_branch()) {
        HAMMER_CHECK(isa<BlockExpr>(e) || isa<IfExpr>(e),
            "Invalid else branch of type {} (must be either a block or "
            "another if statement).");
    }
    visit_node(expr);
}

void SemanticChecker::visit_var_decl(const NodePtr<VarDecl>& var) {
    if (var->is_const() && var->initializer() == nullptr) {
        diag_.reportf(
            Diagnostics::Error, var->start(), "Constants must be initialized.");
        var->has_error(true);
    }
    visit_decl(var);
}

void SemanticChecker::visit_binary_expr(const NodePtr<BinaryExpr>& expr) {
    HAMMER_CHECK(expr->left(), "Binary expression without a left child.");
    HAMMER_CHECK(expr->right(), "Binary expression without a right child.");

    if (expr->operation() == BinaryOperator::Assign) {
        if (!isa<VarExpr>(expr->left()) && !isa<DotExpr>(expr->left())
            && !isa<TupleMemberExpr>(expr->left())
            && !isa<IndexExpr>(expr->left())) {

            diag_.reportf(Diagnostics::Error, expr->left()->start(),
                "Invalid left hand side operator {} for an assignment.",
                to_string(expr->type()));
            expr->has_error(true);
            return;
        }

        if (const NodePtr<VarExpr> lhs = try_cast<VarExpr>(expr->left());
            lhs && !lhs->has_error()) {

            struct AssignmentChecker {
                const NodePtr<VarExpr>& lhs_;
                SemanticChecker& checker_;

                AssignmentChecker(
                    const NodePtr<VarExpr>& lhs, SemanticChecker& checker)
                    : lhs_(lhs)
                    , checker_(checker) {}

                void visit_var_decl(const NodePtr<VarDecl>& decl) {
                    if (decl->is_const()) {
                        checker_.diag_.reportf(Diagnostics::Error,
                            lhs_->start(),
                            "Cannot assign to the constant '{}'.",
                            checker_.strings_.value(decl->name()));
                        lhs_->has_error(true);
                    }
                }

                void visit_param_decl(const NodePtr<ParamDecl>&) {}

                void visit_func_decl(const NodePtr<FuncDecl>& decl) {
                    checker_.diag_.reportf(Diagnostics::Error, lhs_->start(),
                        "Cannot assign to the function '{}'.",
                        checker_.strings_.value(decl->name()));
                    lhs_->has_error(true);
                }

                void visit_import_decl(const NodePtr<ImportDecl>& decl) {
                    checker_.diag_.reportf(Diagnostics::Error, lhs_->start(),
                        "Cannot assign to the imported symbol '{}'.",
                        checker_.strings_.value(decl->name()));
                    lhs_->has_error(true);
                }
            } check_assign(lhs, *this);

            auto entry = lhs->resolved_symbol();
            HAMMER_ASSERT(entry, "Symbol was not resolved.");
            visit(entry->decl(), check_assign);
        }
    }
    visit_expr(expr);
}

void SemanticChecker::visit_node(const NodePtr<>& node) {
    traverse_children(node, [&](auto&& child) { check(child); });
}

} // namespace hammer::compiler
