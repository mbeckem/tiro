#include "hammer/compiler/analyzer.hpp"

#include "hammer/ast/node_visit.hpp"
#include "hammer/core/error.hpp"
#include "hammer/core/overloaded.hpp"

namespace hammer {

ast::Scope* Analyzer::as_scope(ast::Node& node) {
    return ast::visit(node, [](auto& n) -> ast::Scope* {
        using type = remove_cvref_t<decltype(n)>;
        if constexpr (std::is_base_of_v<ast::Scope, type>) {
            return &n;
        }
        return nullptr;
    });
}

Analyzer::Analyzer(StringTable& strings, Diagnostics& diag)
    : strings_(strings)
    , diag_(diag) {}

void Analyzer::analyze(ast::Root* root) {
    HAMMER_ASSERT_NOT_NULL(root);
    HAMMER_ASSERT(root->child(), "Root must have a child.");

    build_scopes(root->child(), root);
    resolve_symbols(root);
    check_values(root, false);
    check_structure(root);
}

void Analyzer::build_scopes(ast::Node* node, ast::Scope* current_scope) {
    HAMMER_ASSERT_NOT_NULL(current_scope);

    if (!node || node->has_error())
        return;

    if (ast::Decl* decl = try_cast<ast::Decl>(node)) {
        bool inserted = current_scope->insert(decl);
        if (!inserted) {
            diag_.reportf(Diagnostics::error, decl->pos(),
                          "The name '{}' has already been defined in this scope.",
                          strings_.value(decl->name()));
            decl->has_error(true);
        }
    }

    if (ast::VarExpr* var = try_cast<ast::VarExpr>(node)) {
        var->surrounding_scope(current_scope);
    }

    ast::Scope* node_as_scope = as_scope(*node);
    ast::Scope* next_scope;
    if (node_as_scope) {
        node_as_scope->parent_scope(current_scope);
        next_scope = node_as_scope;
    } else {
        next_scope = current_scope;
    }

    for (ast::Node& child : node->children()) {
        build_scopes(&child, next_scope);
    }
}

void Analyzer::resolve_symbols(ast::Node* node) {
    if (!node || node->has_error())
        return;

    auto visit_children = [&](ast::Node& n) {
        for (ast::Node& child : n.children()) {
            resolve_symbols(&child);
        }
    };

    Overloaded visitor = {[&](ast::VarDecl& var) {
                              // The symbol is *not* active in its own initializer.
                              visit_children(var);
                              var.active(true);
                          },
                          [&](ast::File& file) {
                              for (ast::Node& child : file.children()) {
                                  // Function declarations in file scope are always active.
                                  // TODO: Variables / constants / classes
                                  if (auto* func = try_cast<ast::FuncDecl>(&child))
                                      func->active(true);
                              }
                              visit_children(file);
                          },
                          [&](ast::Decl& sym) {
                              if (!sym.active())
                                  sym.active(true);

                              visit_children(sym);
                          },
                          [&](ast::VarExpr& var) {
                              resolve_var(&var);
                              visit_children(var);
                          },
                          [&](ast::Node& other) { visit_children(other); }};
    ast::visit(*node, visitor);
}

void Analyzer::resolve_var(ast::VarExpr* var) {
    HAMMER_ASSERT_NOT_NULL(var);

    ast::Scope* scope = var->surrounding_scope();
    HAMMER_CHECK(scope, "Scope was not set.");
    HAMMER_CHECK(!var->decl(), "Symbol has already been resolved.");
    HAMMER_CHECK(var->name(), "Var expr without a name.");

    ast::Decl* sym = scope->find(var->name()).first;
    if (!sym) {
        diag_.reportf(Diagnostics::error, var->pos(), "Undefined symbol: '{}'.",
                      strings_.value(var->name()));
        var->has_error(true);
        return;
    }
    var->decl(sym);

    if (!sym->active()) {
        diag_.reportf(Diagnostics::error, var->pos(),
                      "Symbol '{}' referenced before its declaration in the current scope.",
                      strings_.value(var->name()));
        var->has_error(true);
        return;
    }
}

void Analyzer::check_values(ast::Node* node, bool required) {
    if (!node || node->has_error())
        return;

    Overloaded visitor = {
        // A block used by other expressions must have an expression as its last statement
        // and that expression must produce a value.
        [&](ast::BlockExpr& expr) {
            const size_t statements = expr.stmt_count();
            if (statements > 0) {
                for (size_t i = 0; i < statements - 1; ++i) {
                    check_values(expr.get_stmt(i), false);
                }

                ast::Stmt* last_child = expr.get_stmt(statements - 1);
                if (required && !isa<ast::ExprStmt>(last_child)) {
                    diag_.report(Diagnostics::error, last_child->pos(),
                                 "This block must produce a value: the last statement "
                                 "must be an expression.");
                    expr.has_error(true);
                }

                check_values(last_child, required);
                if (auto* last_expr = try_cast<ast::ExprStmt>(last_child);
                    last_expr && last_expr->expression()->has_value()) {
                    expr.has_value(true);
                    last_expr->used(true);
                }
            } else if (required) {
                diag_.report(Diagnostics::error, expr.pos(),
                             "This block must produce a value: it cannot be empty.");
                expr.has_error(true);
            }

            // Act as if we had a value, even if we had an error above. Parent expressions can continue checking.
            if (required && !expr.has_value())
                expr.has_value(true);
        },
        // If an if expr is used by other expressions, it must have two branches and both must produce a value.
        [&](ast::IfExpr& expr) {
            check_values(expr.condition(), true);
            check_values(expr.then_branch(), required);
            check_values(expr.else_branch(), required);

            if (required && !expr.else_branch()) {
                diag_.report(Diagnostics::error, expr.pos(),
                             "This if expression must produce a value: it must have an "
                             "'else' branch.");
                expr.has_error(true);
            }

            if (expr.then_branch()->has_value() && expr.else_branch()
                && expr.else_branch()->has_value()) {
                expr.has_value(true);
            }

            // Act as if we had a value, even if we had an error above. Parent expressions can continue checking.
            if (required && !expr.has_value())
                expr.has_value(true);
        },
        // All other expressions always return a value
        [&](ast::Expr& expr) {
            for (auto& child : expr.children()) {
                check_values(&child, true);
            }
            expr.has_value(true);
        },
        [&](ast::WhileStmt& stmt) {
            check_values(stmt.condition(), true);
            check_values(stmt.body(), false);
        },
        [&](ast::ForStmt& stmt) {
            check_values(stmt.decl(), false);
            check_values(stmt.condition(), true);
            check_values(stmt.step(), false);
            check_values(stmt.body(), false);
        },
        [&](ast::ExprStmt& stmt) { check_values(stmt.expression(), required); },
        [&](ast::VarDecl& decl) { check_values(decl.initializer(), true); },
        [&](ast::Node& n) {
            for (auto& child : n.children()) {
                check_values(&child, false);
            }
        }};

    ast::visit(*node, visitor);
}

void Analyzer::check_structure(ast::Node* node) {
    if (!node || node->has_error())
        return;

    auto visit_children = [&](ast::Node& n) {
        for (ast::Node& child : n.children()) {
            check_structure(&child);
        }
    };

    Overloaded visitor = {
        [&](ast::Root& r) {
            HAMMER_ASSERT(r.child(), "Root does not have a child.");
            visit_children(r);
        },
        [&](ast::File& f) {
            const size_t items = f.item_count();
            for (size_t i = 0; i < items; ++i) {
                ast::Node* child = f.get_item(i);
                if (!isa<ast::FuncDecl>(child) && !isa<ast::ImportDecl>(child)) {
                    // TODO: More items are allowed

                    diag_.reportf(
                        Diagnostics::error, child->pos(),
                        "Invalid top level construct of type {}. Only functions and imports are "
                        "allowed for now.",
                        to_string(child->kind()));
                    f.has_error(true);
                    return;
                }
            }
            visit_children(f);
        },
        [&](ast::IfExpr& i) {
            if (auto* e = i.else_branch()) {
                HAMMER_CHECK(isa<ast::BlockExpr>(e) || isa<ast::IfExpr>(e),
                             "Invalid else branch of type {} (must be either a block or "
                             "another if statement).");
            }
            visit_children(i);
        },
        [&](ast::BinaryExpr& e) {
            HAMMER_ASSERT(e.left_child(), "Binary expression without a left child.");

            if (e.operation() == ast::BinaryOperator::assign) {
                if (!isa<ast::VarExpr>(e.left_child()) && !isa<ast::DotExpr>(e.left_child())
                    && !isa<ast::IndexExpr>(e.left_child())) {

                    diag_.reportf(Diagnostics::error, e.left_child()->pos(),
                                  "Invalid left hand side operator {} for an assignment.",
                                  to_string(e.kind()));
                    e.has_error(true);
                    return;
                }

                if (ast::VarExpr* lhs = try_cast<ast::VarExpr>(e.left_child());
                    lhs && !lhs->has_error()) {
                    auto check_assign = Overloaded{
                        [&](ast::VarDecl& v) {
                            if (v.is_const()) {
                                diag_.reportf(Diagnostics::error, lhs->pos(),
                                              "Cannot assign to the constant '{}'.",
                                              strings_.value(v.name()));
                                lhs->has_error(true);
                            }
                        },
                        [&](ast::ParamDecl&) {},
                        [&](ast::FuncDecl& f) {
                            diag_.reportf(Diagnostics::error, lhs->pos(),
                                          "Cannot assign to the function '{}'.",
                                          strings_.value(f.name()));
                            lhs->has_error(true);
                        },
                        [&](ast::ImportDecl& i) {
                            diag_.reportf(Diagnostics::error, lhs->pos(),
                                          "Cannot assign to the imported symbol '{}'.",
                                          strings_.value(i.name()));
                            lhs->has_error(true);
                        }};

                    HAMMER_ASSERT(lhs->decl(), "Var expression must have an resolved symbol.");
                    ast::visit(*lhs->decl(), check_assign);
                }
            }
            visit_children(e);
        },
        [&](ast::Node& n) { visit_children(n); }};

    ast::visit(*node, visitor);
}

} // namespace hammer
