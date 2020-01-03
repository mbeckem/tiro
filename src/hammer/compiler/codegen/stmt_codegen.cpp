#include "hammer/compiler/codegen/stmt_codegen.hpp"

namespace hammer::compiler {

StmtCodegen::StmtCodegen(const NodePtr<Stmt>& stmt, FunctionCodegen& func)
    : stmt_(stmt)
    , func_(func)
    , builder_(func.builder())
    , strings_(func.strings())
    , diag_(func.diag()) {}

void StmtCodegen::generate() {
    HAMMER_ASSERT_NOT_NULL(stmt_);
    HAMMER_ASSERT(!stmt_->has_error(), "Invalid node in codegen.");

    visit(stmt_, *this);
}

void StmtCodegen::visit_assert_stmt(const NodePtr<AssertStmt>& s) {
    LabelGroup group(builder_);
    const LabelID assert_ok = group.gen("assert-ok");

    func_.generate_expr_value(s->condition());
    builder_.jmp_true_pop(assert_ok);

    // The expression (in source code form) that failed to return true.
    // TODO: Take the expression from the source code
    {
        InternedString string = strings_.insert("expression");
        const u32 constant_index = module().add_string(string);
        builder_.load_module(constant_index);
    }

    // The optional assertion message.
    if (const auto& msg = s->message()) {
        func_.generate_expr_value(msg);
    } else {
        builder_.load_null();
    }
    builder_.assert_fail();

    builder_.define_label(assert_ok);
}

void StmtCodegen::visit_while_stmt(const NodePtr<WhileStmt>& s) {
    LabelGroup group(builder_);
    const LabelID while_cond = group.gen("while-cond");
    const LabelID while_end = group.gen("while-end");

    builder_.define_label(while_cond);
    func_.generate_expr_value(s->condition());
    builder_.jmp_false_pop(while_end);

    func_.generate_loop_body(while_end, while_cond, s->body_scope(), s->body());
    builder_.jmp(while_cond);

    builder_.define_label(while_end);
}

void StmtCodegen::visit_for_stmt(const NodePtr<ForStmt>& s) {
    LabelGroup group(builder_);
    const LabelID for_cond = group.gen("for-cond");
    const LabelID for_step = group.gen("for-step");
    const LabelID for_end = group.gen("for-end");

    if (const auto& decl = s->decl()) {
        func_.generate_stmt(decl);
    }

    builder_.define_label(for_cond);
    if (const auto& cond = s->condition()) {
        func_.generate_expr_value(cond);
        builder_.jmp_false_pop(for_end);
    } else {
        // Nothing, fall through to body. Equivalent to for (; true; )
    }

    func_.generate_loop_body(for_end, for_step, s->body_scope(), s->body());

    builder_.define_label(for_step);
    if (const auto& step = s->step()) {
        func_.generate_expr(step);
        if (step->expr_type() == ExprType::Value)
            builder_.pop();
    }
    builder_.jmp(for_cond);

    builder_.define_label(for_end);
}

void StmtCodegen::visit_decl_stmt(const NodePtr<DeclStmt>& s) {
    const auto& decl = s->decl();
    HAMMER_ASSERT_NOT_NULL(decl);

    if (const auto& init = decl->initializer()) {
        func_.generate_store(decl->declared_symbol(), init, false);
    }
}

void StmtCodegen::visit_expr_stmt(const NodePtr<ExprStmt>& s) {
    const auto& expr = s->expr();
    HAMMER_ASSERT_NOT_NULL(expr);

    func_.generate_expr(expr);
    if (expr->expr_type() == ExprType::Value)
        builder_.pop();
}

} // namespace hammer::compiler
