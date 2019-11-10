#include "hammer/compiler/codegen/stmt_codegen.hpp"

#include "hammer/ast/node_visit.hpp"

namespace hammer {

StmtCodegen::StmtCodegen(ast::Stmt& stmt, FunctionCodegen& func)
    : stmt_(stmt)
    , func_(func)
    , builder_(func.builder())
    , strings_(func.strings())
    , diag_(func.diag()) {}

void StmtCodegen::generate() {
    HAMMER_ASSERT(!stmt_.has_error(), "Invalid node in codegen.");

    ast::visit(stmt_, [&](auto&& n) { gen_impl(n); });
}

void StmtCodegen::gen_impl(ast::AssertStmt& s) {
    LabelGroup group(builder_);
    const LabelID assert_ok = group.gen("assert-ok");

    func_.generate_expr_value(s.condition());
    builder_.jmp_true_pop(assert_ok);

    // The expression (in source code form) that failed to return true.
    // TODO: Take the expression from the source code
    {
        InternedString string = strings_.insert("expression");
        const u32 constant_index = module().add_string(string);
        builder_.load_module(constant_index);
    }

    // The optional assertion message.
    if (ast::StringLiteral* msg = s.message()) {
        func_.generate_expr_value(msg);
    } else {
        builder_.load_null();
    }
    builder_.assert_fail();

    builder_.define_label(assert_ok);
}

void StmtCodegen::gen_impl(ast::WhileStmt& s) {
    LabelGroup group(builder_);
    const LabelID while_cond = group.gen("while-cond");
    const LabelID while_end = group.gen("while-end");

    builder_.define_label(while_cond);
    func_.generate_expr_value(s.condition());
    builder_.jmp_false_pop(while_end);

    func_.generate_loop_body(while_end, while_cond, s.body());
    builder_.jmp(while_cond);

    builder_.define_label(while_end);
}

void StmtCodegen::gen_impl(ast::ForStmt& s) {
    LabelGroup group(builder_);
    const LabelID for_cond = group.gen("for-cond");
    const LabelID for_step = group.gen("for-step");
    const LabelID for_end = group.gen("for-end");

    if (s.decl()) {
        func_.generate_stmt(s.decl());
    }

    builder_.define_label(for_cond);
    if (s.condition()) {
        func_.generate_expr_value(s.condition());
        builder_.jmp_false_pop(for_end);
    } else {
        // Nothing, fall through to body. Equivalent to for (; true; )
    }

    func_.generate_loop_body(for_end, for_step, s.body());

    builder_.define_label(for_step);
    if (s.step()) {
        func_.generate_expr(s.step());
        if (s.step()->expr_type() == ast::ExprType::Value)
            builder_.pop();
    }
    builder_.jmp(for_cond);

    builder_.define_label(for_end);
}

void StmtCodegen::gen_impl(ast::DeclStmt& s) {
    ast::Expr* init = s.declaration()->initializer();
    if (init) {
        func_.generate_store(s.declaration(), init, false);
    }
}

void StmtCodegen::gen_impl(ast::ExprStmt& s) {
    func_.generate_expr(s.expression());
    if (s.expression()->expr_type() == ast::ExprType::Value && !s.used())
        builder_.pop();
}

} // namespace hammer
