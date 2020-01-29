#include "tiro/codegen/stmt_codegen.hpp"

namespace tiro::compiler {

StmtCodegen::StmtCodegen(NotNull<Stmt*> stmt, FunctionCodegen& func)
    : stmt_(stmt)
    , func_(func)
    , builder_(func.builder())
    , strings_(func.strings())
    , diag_(func.diag()) {}

void StmtCodegen::generate() {
    TIRO_ASSERT(!stmt_->has_error(), "Invalid node in codegen.");

    visit(TIRO_NN(stmt_), *this);
}

void StmtCodegen::visit_assert_stmt(AssertStmt* s) {
    LabelGroup group(builder_);
    const LabelID assert_ok = group.gen("assert-ok");

    func_.generate_expr_value(TIRO_NN(s->condition()));
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
        TIRO_ASSERT(isa<StringLiteral>(msg) || isa<InterpolatedStringExpr>(msg),
            "Invalid expression type used as assert message, must be a "
            "string.");
        func_.generate_expr_value(TIRO_NN(msg));
    } else {
        builder_.load_null();
    }
    builder_.assert_fail();

    builder_.define_label(assert_ok);
}

void StmtCodegen::visit_while_stmt(WhileStmt* s) {
    LabelGroup group(builder_);
    const LabelID while_cond = group.gen("while-cond");
    const LabelID while_end = group.gen("while-end");

    builder_.define_label(while_cond);
    func_.generate_expr_value(TIRO_NN(s->condition()));
    builder_.jmp_false_pop(while_end);

    func_.generate_loop_body(
        while_end, while_cond, s->body_scope(), TIRO_NN(s->body()));
    builder_.jmp(while_cond);

    builder_.define_label(while_end);
}

void StmtCodegen::visit_for_stmt(ForStmt* s) {
    LabelGroup group(builder_);
    const LabelID for_cond = group.gen("for-cond");
    const LabelID for_step = group.gen("for-step");
    const LabelID for_end = group.gen("for-end");

    if (const auto& decl = s->decl()) {
        func_.generate_stmt(TIRO_NN(decl));
    }

    builder_.define_label(for_cond);
    if (const auto& cond = s->condition()) {
        func_.generate_expr_value(TIRO_NN(cond));
        builder_.jmp_false_pop(for_end);
    } else {
        // Nothing, fall through to body. Equivalent to for (; true; )
    }

    func_.generate_loop_body(
        for_end, for_step, s->body_scope(), TIRO_NN(s->body()));

    builder_.define_label(for_step);
    if (const auto& step = s->step()) {
        func_.generate_expr_ignore(TIRO_NN(step));
    }
    builder_.jmp(for_cond);

    builder_.define_label(for_end);
}

void StmtCodegen::visit_decl_stmt(DeclStmt* s) {
    const auto bindings = TIRO_NN(s->bindings());

    struct BindingVisitor {
        CodeBuilder* builder;
        FunctionCodegen* gen;

        void visit_var_binding(VarBinding* b) {
            const auto var = TIRO_NN(b->var());
            const auto entry = TIRO_NN(var->declared_symbol());
            if (const auto& init = b->init()) {
                gen->generate_expr_value(TIRO_NN(init));
                gen->generate_store(entry);
            }
        }

        void visit_tuple_binding(TupleBinding* b) {
            const auto vars = TIRO_NN(b->vars());

            // TODO: If the initializer is a tuple literal (i.e. known contents at compile time)
            // we can skip generating the complete tuple and assign the individual variables directly.
            // This should also be done for tuple assignments (see expr_codegen.cpp).
            if (const auto& init = b->init()) {
                gen->generate_expr_value(TIRO_NN(init));

                const size_t var_count = vars->size();

                // 0 variables on the left side - useless but valid syntax.
                if (var_count == 0) {
                    builder->pop();
                    return;
                }

                for (size_t i = 0; i < var_count; ++i) {
                    const auto var = TIRO_NN(vars->get(i));

                    if (i != var_count - 1) {
                        builder->dup();
                    }

                    builder->load_tuple_member(i);
                    gen->generate_store(var->declared_symbol());
                }
            }
        }
    } visitor{&builder_, &func_};

    for (auto binding : bindings->entries()) {
        visit(TIRO_NN(binding), visitor);
    }
}

void StmtCodegen::visit_expr_stmt(ExprStmt* s) {
    const NotNull expr = TIRO_NN(s->expr());

    // Ignoring is not a problem here - expression statements that are used
    // as values (i.e. last statement in a block) are compiled differently in the
    // ExprCodegen class.
    func_.generate_expr_ignore(expr);
}

} // namespace tiro::compiler
