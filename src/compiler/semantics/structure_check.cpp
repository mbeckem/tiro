#include "compiler/semantics/structure_check.hpp"

#include "common/not_null.hpp"
#include "common/string_table.hpp"
#include "compiler/ast/ast.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/reset_value.hpp"
#include "compiler/semantics/symbol_table.hpp"

namespace tiro {

namespace {

class StructureChecker final : public DefaultNodeVisitor<StructureChecker> {
public:
    explicit StructureChecker(
        const SymbolTable& symbols, const StringTable& strings, Diagnostics& diag);
    ~StructureChecker();

    StructureChecker(const StructureChecker&) = delete;
    StructureChecker& operator=(const StructureChecker&) = delete;

    void check(AstNode* node);

    void visit_file(NotNull<AstFile*> file) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_binding(NotNull<AstBinding*> binding) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_func_decl(NotNull<AstFuncDecl*> decl) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_for_stmt(NotNull<AstForStmt*> stmt) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_while_stmt(NotNull<AstWhileStmt*> stmt) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_if_expr(NotNull<AstIfExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_binary_expr(NotNull<AstBinaryExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_continue_expr(NotNull<AstContinueExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_break_expr(NotNull<AstBreakExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_return_expr(NotNull<AstReturnExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE;

    void visit_node(NotNull<AstNode*> node) TIRO_NODE_VISITOR_OVERRIDE;

private:
    bool check_assignment_lhs(NotNull<AstExpr*> expr, bool allow_tuple);
    bool check_assignment_path(NotNull<AstExpr*> expr);
    bool check_assignment_var(NotNull<AstVarExpr*> expr);

    ResetValue<AstNode*> enter_loop(NotNull<AstNode*> loop);
    ResetValue<AstNode*> enter_func(NotNull<AstNode*> func);

private:
    const SymbolTable& symbols_;
    const StringTable& strings_;
    Diagnostics& diag_;

    AstNode* current_function_ = nullptr;
    AstNode* current_loop_ = nullptr;
};

} // namespace

StructureChecker::StructureChecker(
    const SymbolTable& symbols, const StringTable& strings, Diagnostics& diag)
    : symbols_(symbols)
    , strings_(strings)
    , diag_(diag) {}

StructureChecker::~StructureChecker() {}

void StructureChecker::check(AstNode* node) {
    if (node && !node->has_error())
        visit(TIRO_NN(node), *this);
}

void StructureChecker::visit_file(NotNull<AstFile*> file) {
    for (auto stmt : file->items()) {
        switch (stmt->type()) {
        case AstNodeType::EmptyStmt:
        case AstNodeType::DeclStmt:
            break;

        default:
            diag_.reportf(Diagnostics::Error, stmt->source(),
                "Invalid top level construct of type {}. Only "
                "declarations of imports, variables and functions are allowed for now.",
                to_string(stmt->type()));
            stmt->has_error(true);
            return;
        }
    }

    visit_node(file);
}

void StructureChecker::visit_binding(NotNull<AstBinding*> binding) {
    const bool has_init = binding->init() != nullptr;

    if (binding->is_const() && !has_init) {
        diag_.reportf(Diagnostics::Error, binding->source(), "Constant is not being initialized.");
        binding->has_error(true);
    }

    visit_node(binding);
}

void StructureChecker::visit_func_decl(NotNull<AstFuncDecl*> decl) {
    auto reset_func = enter_func(decl);
    visit_decl(decl);
}

void StructureChecker::visit_for_stmt(NotNull<AstForStmt*> stmt) {
    auto reset_loop = enter_loop(stmt);
    visit_stmt(stmt);
}

void StructureChecker::visit_while_stmt(NotNull<AstWhileStmt*> stmt) {
    auto reset_loop = enter_loop(stmt);
    visit_stmt(stmt);
}

void StructureChecker::visit_if_expr(NotNull<AstIfExpr*> expr) {
    if (const auto& e = expr->else_branch()) {
        TIRO_CHECK(is_instance<AstBlockExpr>(e) || is_instance<AstIfExpr>(e),
            "Invalid else branch of type {} (must be either a block or "
            "another if statement).");
    }

    visit_node(expr);
}

void StructureChecker::visit_binary_expr(NotNull<AstBinaryExpr*> expr) {
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
        const auto lhs = TIRO_NN(expr->left());
        if (lhs->has_error() || !check_assignment_lhs(lhs, allow_tuple)) {
            expr->has_error(true);
        }
        break;
    }
    default:
        break;
    }

    visit_expr(expr);
}

void StructureChecker::visit_continue_expr(NotNull<AstContinueExpr*> expr) {
    if (!current_loop_) {
        diag_.reportf(Diagnostics::Error, expr->source(),
            "Continue expressions are not allowed outside a loop.");
        expr->has_error(true);
        return;
    }

    visit_expr(expr);
}

void StructureChecker::visit_break_expr(NotNull<AstBreakExpr*> expr) {
    if (!current_loop_) {
        diag_.reportf(Diagnostics::Error, expr->source(),
            "Break expressions are not allowed outside a loop.");
        expr->has_error(true);
        return;
    }

    visit_expr(expr);
}

void StructureChecker::visit_return_expr(NotNull<AstReturnExpr*> expr) {
    if (!current_function_) {
        diag_.reportf(Diagnostics::Error, expr->source(),
            "Return expressions are not allowed outside a function.");
        expr->has_error(true);
        return;
    }

    visit_expr(expr);
}

void StructureChecker::visit_node(NotNull<AstNode*> node) {
    node->traverse_children([&](AstNode* child) { check(child); });
}

bool StructureChecker::check_assignment_lhs(NotNull<AstExpr*> expr, bool allow_tuple) {
    switch (expr->type()) {
    case AstNodeType::PropertyExpr:
    case AstNodeType::ElementExpr:
        return check_assignment_path(expr);

    case AstNodeType::VarExpr: {
        return check_assignment_var(must_cast<AstVarExpr>(expr));
    }

    case AstNodeType::TupleLiteral: {
        auto tuple = must_cast<AstTupleLiteral>(expr);
        if (!allow_tuple) {
            diag_.report(Diagnostics::Error, tuple->source(),
                "Tuple assignments are not supported in this context.");
            tuple->has_error(true);
            return false;
        }

        for (auto item : tuple->items()) {
            if (!check_assignment_lhs(TIRO_NN(item), false)) {
                tuple->has_error(true);
                return false;
            }
        }

        return true;
    }

    default:
        diag_.reportf(Diagnostics::Error, expr->source(),
            "Cannot use operand of type {} as the left hand side of an "
            "assignment.",
            to_string(expr->type()));
        expr->has_error(true);
        return false;
    }
}

bool StructureChecker::check_assignment_path(NotNull<AstExpr*> expr) {
    struct Visitor : DefaultNodeVisitor<Visitor, bool&> {
        StructureChecker& self;

        Visitor(StructureChecker& self_)
            : self(self_) {}

        void
        visit_property_expr(NotNull<AstPropertyExpr*> expr, bool& ok) TIRO_NODE_VISITOR_OVERRIDE {
            switch (expr->access_type()) {
            case AccessType::Optional:
                self.diag_.reportf(Diagnostics::Error, expr->source(),
                    "Optional property expressions are not supported as left hand "
                    "side of an assignment expression.");
                expr->has_error(true);
                ok = false;
                return;
            case AccessType::Normal:
                break;
            }

            ok = self.check_assignment_path(TIRO_NN(expr->instance()));
        }

        void
        visit_element_expr(NotNull<AstElementExpr*> expr, bool& ok) TIRO_NODE_VISITOR_OVERRIDE {
            switch (expr->access_type()) {
            case AccessType::Optional:
                self.diag_.reportf(Diagnostics::Error, expr->source(),
                    "Optional element expressions are not supported as left hand "
                    "side of an assignment expression.");
                expr->has_error(true);
                ok = false;
                return;
            case AccessType::Normal:
                break;
            }

            ok = self.check_assignment_path(TIRO_NN(expr->instance()));
        }

        void visit_call_expr(NotNull<AstCallExpr*> expr, bool& ok) TIRO_NODE_VISITOR_OVERRIDE {
            switch (expr->access_type()) {
            case AccessType::Optional:
                self.diag_.reportf(Diagnostics::Error, expr->source(),
                    "Optional call expressions are not supported as left hand "
                    "side of an assignment expression.");
                expr->has_error(true);
                ok = false;
                return;
            case AccessType::Normal:
                break;
            }

            ok = self.check_assignment_path(TIRO_NN(expr->func()));
        }

        void
        visit_expr([[maybe_unused]] NotNull<AstExpr*> expr, bool& ok) TIRO_NODE_VISITOR_OVERRIDE {
            ok = true;
        }
    };

    bool ok = false;
    visit(expr, Visitor(*this), ok);
    return ok;
}

bool StructureChecker::check_assignment_var(NotNull<AstVarExpr*> expr) {
    auto symbol_id = symbols_.get_ref(expr->id());
    auto symbol = symbols_[symbol_id];

    switch (symbol->type()) {
    case SymbolType::Import:
        diag_.reportf(Diagnostics::Error, expr->source(),
            "Cannot assign to the imported symbol '{}'.", strings_.value(symbol->name()));
        expr->has_error(true);
        return false;
    case SymbolType::Function:
        diag_.reportf(Diagnostics::Error, expr->source(), "Cannot assign to the function '{}'.",
            strings_.value(symbol->name()));
        expr->has_error(true);
        return false;
    case SymbolType::Parameter:
        return true;
    case SymbolType::Variable:
        if (symbol->is_const()) {
            diag_.reportf(Diagnostics::Error, expr->source(), "Cannot assign to the constant '{}'.",
                strings_.dump(symbol->name()));
            expr->has_error(true);
            return false;
        }
        return true;
    case SymbolType::TypeSymbol:
        diag_.reportf(Diagnostics::Error, expr->source(), "Cannot assign to the type '{}'.",
            strings_.value(symbol->name()));
        expr->has_error(true);
        return false;
    }

    TIRO_UNREACHABLE("Invalid symbol type.");
}

ResetValue<AstNode*> StructureChecker::enter_loop(NotNull<AstNode*> loop) {
    return replace_value(current_loop_, loop.get());
}

ResetValue<AstNode*> StructureChecker::enter_func(NotNull<AstNode*> func) {
    return replace_value(current_function_, func.get());
}

void check_structure(
    AstNode* node, const SymbolTable& symbols, const StringTable& strings, Diagnostics& diag) {
    StructureChecker checker(symbols, strings, diag);
    checker.check(node);
}

} // namespace tiro
