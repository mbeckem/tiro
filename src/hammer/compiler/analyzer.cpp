#include "hammer/compiler/analyzer.hpp"

#include "hammer/compiler/symbol_table.hpp"
#include "hammer/core/overloaded.hpp"

namespace hammer::compiler {

namespace {

class TypeChecker final : public DefaultNodeVisitor<TypeChecker, bool> {
public:
    TypeChecker(Diagnostics& diag)
        : diag_(diag) {}

    void check(const NodePtr<>& node, bool requires_value) {
        // TODO we might still be able to recurse into child nodes and check them,
        // even if the parent node contains errors?

        if (!node || node->has_error())
            return;

        visit(node, *this, requires_value);
    };

    // A block used by other expressions must have an expression as its last statement
    // and that expression must produce a value.
    void visit_block_expr(const NodePtr<BlockExpr>& expr,
        bool requires_value) HAMMER_VISITOR_OVERRIDE {
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
                // FIXME integerate into codegen last_expr->used(true);
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
    void visit_if_expr(const NodePtr<IfExpr>& expr, bool requires_value) {
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

    void visit_return_expr(const NodePtr<ReturnExpr>& expr,
        [[maybe_unused]] bool requires_value) HAMMER_VISITOR_OVERRIDE {
        check(expr->inner(), true);
        expr->expr_type(ExprType::Never);
    }

    // TODO this should have a case for every existing expr type (no catch all)
    void visit_expr(const NodePtr<Expr>& expr,
        [[maybe_unused]] bool requires_value) HAMMER_VISITOR_OVERRIDE {
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
        const bool expr_returns = !(isa<ReturnExpr>(expr)
                                    || isa<ContinueExpr>(expr)
                                    || isa<BreakExpr>(expr));
        expr->expr_type(expr_returns ? ExprType::Value : ExprType::Never);
    }

    void visit_assert_stmt(const NodePtr<AssertStmt>& stmt,
        [[maybe_unused]] bool requires_value) HAMMER_VISITOR_OVERRIDE {
        check(stmt->condition(), true);
        check(stmt->message(), true);
    }

    void visit_while_stmt(const NodePtr<WhileStmt>& stmt,
        [[maybe_unused]] bool requires_value) HAMMER_VISITOR_OVERRIDE {
        check(stmt->condition(), true);
        check(stmt->body(), false);
    }

    void visit_for_stmt(const NodePtr<ForStmt>& stmt,
        [[maybe_unused]] bool requires_value) HAMMER_VISITOR_OVERRIDE {
        check(stmt->decl(), false);
        check(stmt->condition(), true);
        check(stmt->step(), false);
        check(stmt->body(), false);
    }

    void visit_expr_stmt(const NodePtr<ExprStmt>& stmt,
        bool requires_value) HAMMER_VISITOR_OVERRIDE {
        check(stmt->expr(), requires_value);
    }

    void visit_var_decl(const NodePtr<VarDecl>& decl,
        [[maybe_unused]] bool requires_value) HAMMER_VISITOR_OVERRIDE {
        check(decl->initializer(), true);
    }

    void visit_node(const NodePtr<>& node,
        [[maybe_unused]] bool requires_value) HAMMER_VISITOR_OVERRIDE {
        traverse_children(
            node, [&](const NodePtr<>& child) { check(child, false); });
    }

private:
    Diagnostics& diag_;
};

class ScopeBuilder final : public DefaultNodeVisitor<ScopeBuilder> {
    template<typename T>
    struct [[nodiscard]] ResetValue {
        T& location_;
        T old_;

        ResetValue(T & location, T old)
            : location_(location)
            , old_(std::move(old)) {}

        ~ResetValue() { location_ = std::move(old_); }

        ResetValue(const ResetValue&) = delete;
        ResetValue& operator=(const ResetValue&) = delete;
    };

public:
    explicit ScopeBuilder(SymbolTable& symbols, StringTable& strings,
        Diagnostics& diag, ScopePtr global_scope)
        : symbols_(symbols)
        , strings_(strings)
        , diag_(diag)
        , global_scope_(global_scope)
        , current_scope_(nullptr) {}

    void dispatch(const NodePtr<>& node) {
        if (node && !node->has_error()) {
            // Perform type specific actions.
            visit(node, *this);
        }
    }

    void visit_root(const NodePtr<Root>& root) HAMMER_VISITOR_OVERRIDE {
        root->root_scope(global_scope_);

        auto exit_scope = enter_scope(global_scope_);
        dispatch_children(root);
    }

    void visit_file(const NodePtr<File>& file) HAMMER_VISITOR_OVERRIDE {
        const auto scope = symbols_.create_scope(
            ScopeType::File, current_scope_, current_func_);
        file->file_scope(scope);

        auto exit_scope = enter_scope(scope);
        dispatch_children(file);
    }

    void
    visit_func_decl(const NodePtr<FuncDecl>& func) HAMMER_VISITOR_OVERRIDE {
        // TODO: Decls should always have a valid name (?). Dont make anon functions decls.
        if (func->name())
            add_decl(func);

        auto exit_func = enter_func(func);

        const auto param_scope = symbols_.create_scope(
            ScopeType::Parameters, current_scope_, current_func_);
        func->param_scope(param_scope);

        const auto body_scope = symbols_.create_scope(
            ScopeType::FunctionBody, param_scope, current_func_);
        func->body_scope(body_scope);

        auto exit_param_scope = enter_scope(param_scope);
        dispatch(func->params());

        auto exit_body_scope = enter_scope(body_scope);
        dispatch(func->body());
    }

    void visit_decl(const NodePtr<Decl>& decl) HAMMER_VISITOR_OVERRIDE {
        // TODO: Decls should always have a valid name.
        if (decl->name())
            add_decl(decl);

        dispatch_children(decl);
    }

    void visit_for_stmt(const NodePtr<ForStmt>& stmt) HAMMER_VISITOR_OVERRIDE {
        const auto decl_scope = symbols_.create_scope(
            ScopeType::ForStmtDecls, current_scope_, current_func_);
        stmt->decl_scope(decl_scope);

        const auto body_scope = symbols_.create_scope(
            ScopeType::LoopBody, decl_scope, current_func_);
        stmt->body_scope(body_scope);

        auto exit_decl_scope = enter_scope(decl_scope);
        dispatch(stmt->decl());
        dispatch(stmt->condition());
        dispatch(stmt->step());

        auto exit_body_scope = enter_scope(body_scope);
        dispatch(stmt->body());
    }

    void
    visit_while_stmt(const NodePtr<WhileStmt>& stmt) HAMMER_VISITOR_OVERRIDE {
        const auto body_scope = symbols_.create_scope(
            ScopeType::LoopBody, current_scope_, current_func_);
        stmt->body_scope(body_scope);

        dispatch(stmt->condition());

        auto exit_body_scope = enter_scope(body_scope);
        dispatch(stmt->body());
    }

    void
    visit_block_expr(const NodePtr<BlockExpr>& expr) HAMMER_VISITOR_OVERRIDE {
        const auto scope = symbols_.create_scope(
            ScopeType::Block, current_scope_, current_func_);
        expr->block_scope(scope);

        auto exit_scope = enter_scope(scope);
        visit_expr(expr);
    }

    void visit_var_expr(const NodePtr<VarExpr>& expr) HAMMER_VISITOR_OVERRIDE {
        expr->surrounding_scope(current_scope_);
        visit_expr(expr);
    }

    void visit_node(const NodePtr<>& node) HAMMER_VISITOR_OVERRIDE {
        dispatch_children(node);
    }

private:
    void add_decl(const NodePtr<Decl>& decl) {
        HAMMER_ASSERT_NOT_NULL(decl);
        HAMMER_ASSERT(current_scope_, "Not inside a scope.");

        auto entry = current_scope_->insert(decl);
        if (!entry) {
            diag_.reportf(Diagnostics::Error, decl->start(),
                "The name '{}' has already been declared in this scope.",
                strings_.value(decl->name()));
            return;
        }

        decl->declared_symbol(entry);
    }

    [[nodiscard]] ResetValue<ScopePtr> enter_scope(ScopePtr new_scope) {
        ScopePtr old_scope = std::exchange(
            current_scope_, std::move(new_scope));
        return {current_scope_, std::move(old_scope)};
    }

    [[nodiscard]] ResetValue<NodePtr<FuncDecl>>
    enter_func(NodePtr<FuncDecl> new_func) {
        NodePtr<FuncDecl> old_func = std::exchange(
            current_func_, std::move(new_func));
        return {current_func_, std::move(old_func)};
    }

    void dispatch_children(const NodePtr<>& node) {
        if (node) {
            traverse_children(node, [&](auto&& child) { dispatch(child); });
        }
    }

private:
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;

    ScopePtr global_scope_;
    ScopePtr current_scope_;
    NodePtr<FuncDecl> current_func_;
};

class SymbolResolver final : public DefaultNodeVisitor<SymbolResolver> {
public:
    explicit SymbolResolver(
        SymbolTable& symbols, StringTable& strings, Diagnostics& diag)
        : symbols_(symbols)
        , strings_(strings)
        , diag_(diag) {}

    void dispatch(const NodePtr<>& node) {
        if (node && !node->has_error()) {
            visit(node, *this);
        }
    }

    void visit_decl(const NodePtr<Decl>& decl) HAMMER_VISITOR_OVERRIDE {
        // TODO classes will also be active in their bodies
        const bool active_in_children = isa<FuncDecl>(decl);
        if (active_in_children) {
            activate(decl);
            visit_node(decl);
        } else {
            visit_node(decl);
            activate(decl);
        }
    }

    void visit_file(const NodePtr<File>& file) HAMMER_VISITOR_OVERRIDE {
        // Function declarations in file scope are always active.
        // TODO: Variables / constants / classes
        // TODO: can use the file scope for this instead
        traverse_children(file, [&](const NodePtr<>& child) {
            if (auto func = try_cast<FuncDecl>(child))
                activate(func);
        });
        visit_node(file);
    }

    void visit_var_expr(const NodePtr<VarExpr>& expr) HAMMER_VISITOR_OVERRIDE {
        auto expr_scope = expr->surrounding_scope();
        HAMMER_CHECK(expr_scope, "Scope was not set for this expression.");
        HAMMER_CHECK(expr->resolved_symbol() == nullptr,
            "Symbol has already been resolved.");
        HAMMER_CHECK(expr->name(), "Variable reference without a name.");

        auto [decl_entry, decl_scope] = expr_scope->find(expr->name());
        if (!decl_entry) {
            diag_.reportf(Diagnostics::Error, expr->start(),
                "Undefined symbol: '{}'.", strings_.value(expr->name()));
            expr->has_error(true);
            return;
        }

        if (decl_scope->function() != expr_scope->function()
            && expr_scope->is_child_of(decl_scope)) {
            // Expr references something within an outer function
            decl_entry->captured(true);
        }

        if (!decl_entry->active()) {
            diag_.reportf(Diagnostics::Error, expr->start(),
                "Symbol '{}' referenced before it became active in the current "
                "scope.",
                strings_.value(expr->name()));
            expr->has_error(true);
            return;
        }

        expr->resolved_symbol(decl_entry);
        visit_expr(expr);
    }

    void visit_node(const NodePtr<>& node) HAMMER_VISITOR_OVERRIDE {
        dispatch_children(node);
    }

private:
    void activate(const NodePtr<Decl>& decl) {
        auto decl_entry = decl->declared_symbol();
        if (!decl_entry) // TODO there should always be a declared symbol in the future
            return;
        decl_entry->active(true);
    }

    void dispatch_children(const NodePtr<>& node) {
        if (node) {
            traverse_children(node, [&](auto&& child) { dispatch(child); });
        }
    }

private:
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;
};

class SemanticChecker final : public DefaultNodeVisitor<SemanticChecker> {
public:
    explicit SemanticChecker(
        SymbolTable& symbols, StringTable& strings, Diagnostics& diag)
        : symbols_(symbols)
        , strings_(strings)
        , diag_(diag) {}

    void check(const NodePtr<>& node) {
        if (node && !node->has_error()) {
            visit(node, *this);
        }
    }

    void visit_root(const NodePtr<Root>& root) HAMMER_VISITOR_OVERRIDE {
        HAMMER_CHECK(root->file(), "Root does not have a file.");
        visit_node(root);
    }

    void visit_file(const NodePtr<File>& file) HAMMER_VISITOR_OVERRIDE {
        const auto items = file->items();
        HAMMER_CHECK(
            items && items->size() > 0, "File does not have any items.");

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

    void visit_if_expr(const NodePtr<IfExpr>& expr) HAMMER_VISITOR_OVERRIDE {
        if (const auto& e = expr->else_branch()) {
            HAMMER_CHECK(isa<BlockExpr>(e) || isa<IfExpr>(e),
                "Invalid else branch of type {} (must be either a block or "
                "another if statement).");
        }
        visit_node(expr);
    }

    void visit_var_decl(const NodePtr<VarDecl>& var) HAMMER_VISITOR_OVERRIDE {
        if (var->is_const() && var->initializer() == nullptr) {
            diag_.reportf(Diagnostics::Error, var->start(),
                "Constants must be initialized.");
            var->has_error(true);
        }
        visit_decl(var);
    }

    void
    visit_binary_expr(const NodePtr<BinaryExpr>& expr) HAMMER_VISITOR_OVERRIDE {
        HAMMER_CHECK(expr->left(), "Binary expression without a left child.");
        HAMMER_CHECK(expr->right(), "Binary expression without a right child.");

        if (expr->operation() == BinaryOperator::Assign) {
            if (!isa<VarExpr>(expr->left()) && !isa<DotExpr>(expr->left())
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
                        checker_.diag_.reportf(Diagnostics::Error,
                            lhs_->start(),
                            "Cannot assign to the function '{}'.",
                            checker_.strings_.value(decl->name()));
                        lhs_->has_error(true);
                    }

                    void visit_import_decl(const NodePtr<ImportDecl>& decl) {
                        checker_.diag_.reportf(Diagnostics::Error,
                            lhs_->start(),
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

    void visit_node(const NodePtr<>& node) HAMMER_VISITOR_OVERRIDE {
        traverse_children(node, [&](auto&& child) { check(child); });
    }

private:
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;
};

}; // namespace

Analyzer::Analyzer(const NodePtr<Root>& root, SymbolTable& symbols,
    StringTable& strings, Diagnostics& diag)
    : root_(root)
    , symbols_(symbols)
    , strings_(strings)
    , diag_(diag)
    , global_scope_(
          symbols_.create_scope(ScopeType::Global, nullptr, nullptr)) {}

void Analyzer::analyze() {
    HAMMER_ASSERT_NOT_NULL(root_);

    build_scopes(root_);
    resolve_symbols(root_);
    resolve_types(root_);
    check_structure(root_);
}

void Analyzer::build_scopes(const NodePtr<>& node) {
    ScopeBuilder builder(symbols_, strings_, diag_, global_scope_);
    builder.dispatch(node);
}

void Analyzer::resolve_symbols(const NodePtr<>& node) {
    SymbolResolver resolver(symbols_, strings_, diag_);
    resolver.dispatch(node);
}

void Analyzer::resolve_types(const NodePtr<>& node) {
    TypeChecker checker(diag_);
    checker.check(node, false);
}

void Analyzer::check_structure(const NodePtr<>& node) {
    SemanticChecker checker(symbols_, strings_, diag_);
    checker.check(node);
}

} // namespace hammer::compiler
