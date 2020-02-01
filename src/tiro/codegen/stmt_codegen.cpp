#include "tiro/codegen/stmt_codegen.hpp"

#include "tiro/codegen/func_codegen.hpp"
#include "tiro/codegen/module_codegen.hpp"

namespace tiro::compiler {

StmtCodegen::StmtCodegen(
    NotNull<Stmt*> stmt, CurrentBasicBlock& bb, FunctionCodegen& func)
    : stmt_(stmt)
    , func_(func)
    , bb_(bb)
    , strings_(func.strings())
    , diag_(func.diag()) {}

void StmtCodegen::generate() {
    TIRO_ASSERT(!stmt_->has_error(), "Invalid node in codegen.");

    visit(TIRO_NN(stmt_), *this);
}

ModuleCodegen& StmtCodegen::module() {
    return func_.module();
}

void StmtCodegen::visit_assert_stmt(AssertStmt* s) {
    func_.generate_expr_value(TIRO_NN(s->condition()), bb_);

    auto assert_ok_block = func_.blocks().make_block(
        strings_.insert("assert-ok"));
    auto assert_fail_block = func_.blocks().make_block(
        strings_.insert("assert-fail"));
    bb_->edge(BasicBlockEdge::make_cond_jump(
        Opcode::JmpTruePop, assert_ok_block, assert_fail_block));

    {
        CurrentBasicBlock nested(TIRO_NN(assert_fail_block));

        // The expression (in source code form) that failed to return true.
        // TODO: Take the expression from the source code
        {
            InternedString string = strings_.insert("expression");
            const u32 constant_index = module().add_string(string);
            nested->append(func_.make_instr<LoadModule>(constant_index));
        }

        // The optional assertion message.
        if (const auto& msg = s->message()) {
            TIRO_ASSERT(
                isa<StringLiteral>(msg) || isa<InterpolatedStringExpr>(msg),
                "Invalid expression type used as assert message, must be a "
                "string.");
            func_.generate_expr_value(TIRO_NN(msg), nested);
        } else {
            nested->append(func_.make_instr<LoadNull>());
        }

        nested->edge(BasicBlockEdge::make_assert_fail());
    }

    bb_.assign(TIRO_NN(assert_ok_block));
}

void StmtCodegen::visit_while_stmt(WhileStmt* s) {
    auto while_cond_block = func_.blocks().make_block(
        strings_.insert("while-cond"));
    auto while_body_block = func_.blocks().make_block(
        strings_.insert("while-body"));
    auto while_end_block = func_.blocks().make_block(
        strings_.insert("while-end"));
    bb_->edge(BasicBlockEdge::make_jump(while_cond_block));

    // Condition block
    {
        CurrentBasicBlock nested(TIRO_NN(while_cond_block));

        func_.generate_expr_value(TIRO_NN(s->condition()), nested);
        nested->edge(BasicBlockEdge::make_cond_jump(
            Opcode::JmpFalsePop, while_end_block, while_body_block));
    }

    // Body block
    {
        CurrentBasicBlock nested(TIRO_NN(while_body_block));
        func_.generate_loop_body(s->body_scope(), TIRO_NN(while_cond_block),
            TIRO_NN(while_end_block), TIRO_NN(s->body()), nested);
        nested->edge(BasicBlockEdge::make_jump(while_cond_block));
    }

    bb_.assign(TIRO_NN(while_end_block));
}

void StmtCodegen::visit_for_stmt(ForStmt* s) {
    // Initial declaration statement
    if (const auto& decl = s->decl()) {
        func_.generate_stmt(TIRO_NN(decl), bb_);
    }

    auto for_cond_block = func_.blocks().make_block(
        strings_.insert("for-cond"));
    auto for_body_block = func_.blocks().make_block(
        strings_.insert("for-body"));
    auto for_step_block = func_.blocks().make_block(
        strings_.insert("for-body"));
    auto for_end_block = func_.blocks().make_block(strings_.insert("for-end"));
    bb_->edge(BasicBlockEdge::make_jump(for_cond_block));

    // Condition block
    {
        CurrentBasicBlock nested(TIRO_NN(for_cond_block));
        if (const auto& cond = s->condition()) {
            func_.generate_expr_value(TIRO_NN(cond), nested);
            nested->edge(BasicBlockEdge::make_cond_jump(
                Opcode::JmpFalsePop, for_end_block, for_body_block));
        } else {
            // Nothing, fall through to body. Equivalent to for (; true; )
            nested->edge(BasicBlockEdge::make_jump(for_body_block));
        }
    }

    // Body block
    {
        CurrentBasicBlock nested(TIRO_NN(for_body_block));
        func_.generate_loop_body(s->body_scope(), TIRO_NN(for_step_block),
            TIRO_NN(for_end_block), TIRO_NN(s->body()), nested);
        nested->edge(BasicBlockEdge::make_jump(for_step_block));
    }

    // Step block
    {
        CurrentBasicBlock nested(TIRO_NN(for_step_block));
        if (const auto& step = s->step()) {
            func_.generate_expr_ignore(TIRO_NN(step), nested);
        }
        nested->edge(BasicBlockEdge::make_jump(for_cond_block));
    }

    bb_.assign(TIRO_NN(for_end_block));
}

void StmtCodegen::visit_decl_stmt(DeclStmt* s) {
    const auto bindings = TIRO_NN(s->bindings());

    struct BindingVisitor {
        CurrentBasicBlock& bb;
        FunctionCodegen* gen;

        void visit_var_binding(VarBinding* b) {
            const auto var = TIRO_NN(b->var());
            const auto entry = TIRO_NN(var->declared_symbol());
            if (const auto& init = b->init()) {
                gen->generate_expr_value(TIRO_NN(init), bb);
                gen->generate_store(entry, bb);
            }
        }

        void visit_tuple_binding(TupleBinding* b) {
            const auto vars = TIRO_NN(b->vars());

            // TODO: If the initializer is a tuple literal (i.e. known contents at compile time)
            // we can skip generating the complete tuple and assign the individual variables directly.
            // This should also be done for tuple assignments (see expr_codegen.cpp).
            if (const auto& init = b->init()) {
                gen->generate_expr_value(TIRO_NN(init), bb);

                const size_t var_count = vars->size();

                // 0 variables on the left side - useless but valid syntax.
                if (var_count == 0) {
                    bb->append(gen->make_instr<Pop>());
                    return;
                }

                for (size_t i = 0; i < var_count; ++i) {
                    const auto var = TIRO_NN(vars->get(i));

                    if (i != var_count - 1) {
                        bb->append(gen->make_instr<Dup>());
                    }

                    bb->append(gen->make_instr<LoadTupleMember>(i));
                    gen->generate_store(var->declared_symbol(), bb);
                }
            }
        }
    } visitor{bb_, &func_};

    for (auto binding : bindings->entries()) {
        visit(TIRO_NN(binding), visitor);
    }
}

void StmtCodegen::visit_expr_stmt(ExprStmt* s) {
    const NotNull expr = TIRO_NN(s->expr());

    // Ignoring is not a problem here - expression statements that are used
    // as values (i.e. last statement in a block) are compiled differently in the
    // ExprCodegen class.
    func_.generate_expr_ignore(expr, bb_);
}

} // namespace tiro::compiler
