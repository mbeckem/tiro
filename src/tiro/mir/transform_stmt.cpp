#include "tiro/mir/transform_stmt.hpp"

#include "tiro/mir/types.hpp"
#include "tiro/semantics/symbol_table.hpp"
#include "tiro/syntax/ast.hpp"

namespace tiro {

StmtTransformer::StmtTransformer(FunctionContext& ctx, CurrentBlock& bb)
    : Transformer(ctx, bb) {}

StmtResult StmtTransformer::dispatch(NotNull<Stmt*> stmt) {
    TIRO_ASSERT(!stmt->has_error(),
        "Nodes with errors must not reach the mir transformation stage.");
    return visit(stmt, *this);
}

StmtResult StmtTransformer::visit_assert_stmt(AssertStmt* stmt) {
    auto cond_result = bb().compile_expr(TIRO_NN(stmt->condition()));
    if (!cond_result)
        return cond_result.failure();

    auto ok_block = ctx().make_block(strings().insert("assert-ok"));
    auto fail_block = ctx().make_block(strings().insert("assert-fail"));
    bb().end(mir::Terminator::make_branch(
        mir::BranchType::IfTrue, *cond_result, ok_block, fail_block));
    ctx().seal(fail_block);
    ctx().seal(ok_block);

    {
        auto nested = ctx().make_current(fail_block);

        // The expression (in source code form) that failed to return true.
        // TODO: Take the expression from the source code
        auto expr_string = strings().insert("expression");
        auto expr_local = nested.compile_rvalue(
            mir::Constant::make_string(expr_string));

        // The message expression is optional (but should evaluate to a string, if present).
        auto message_result = [&]() -> ExprResult {
            if (stmt->message())
                return nested.compile_expr(TIRO_NN(stmt->message()));

            return nested.compile_rvalue(mir::Constant::make_null());
        }();
        if (!message_result)
            return message_result.failure();

        nested.end(mir::Terminator::make_assert_fail(
            expr_local, *message_result, result().exit()));
    }

    bb().assign(ok_block);
    return ok;
}

StmtResult StmtTransformer::visit_decl_stmt(DeclStmt* stmt) {
    auto bindings = TIRO_NN(stmt->bindings());
    if (bindings->size() != 1)
        TIRO_NOT_IMPLEMENTED();

    auto var_binding = try_cast<VarBinding>(bindings->get(0));
    if (!var_binding)
        TIRO_NOT_IMPLEMENTED();

    auto var = TIRO_NN(var_binding->var());
    auto symbol = TIRO_NN(var->declared_symbol());
    if (const auto& init = var_binding->init()) {
        auto local = bb().compile_expr(TIRO_NN(init));
        if (!local)
            return local.failure();

        bb().compile_assign(TIRO_NN(symbol.get()), *local);
    }

    return ok;
}

StmtResult StmtTransformer::visit_empty_stmt([[maybe_unused]] EmptyStmt* stmt) {
    return ok;
}

StmtResult StmtTransformer::visit_expr_stmt(ExprStmt* stmt) {
    auto result = bb().compile_expr(
        TIRO_NN(stmt->expr()), ExprOptions::MaybeInvalid);
    if (!result)
        return result.failure();

    return ok;
}

StmtResult StmtTransformer::visit_for_stmt(ForStmt* stmt) {
    if (auto decl = stmt->decl()) {
        auto decl_result = bb().compile_stmt(TIRO_NN(decl));
        if (!decl_result)
            return decl_result;
    }

    auto cond_block = ctx().make_block(strings().insert("for-cond"));
    auto body_block = ctx().make_block(strings().insert("for-body"));
    auto end_block = ctx().make_block(strings().insert("for-end"));
    bb().end(mir::Terminator::make_jump(cond_block));

    // Compile condition.
    {
        CurrentBlock cond_bb = ctx().make_current(cond_block);
        auto cond_result = compile_loop_cond(
            stmt->condition(), body_block, end_block, cond_bb);
        if (!cond_result) {
            ctx().seal(cond_block);
            bb().assign(cond_block);
            return cond_result;
        }
    }
    ctx().seal(body_block);

    // Compile loop body.
    [[maybe_unused]] auto body_result = [&]() -> StmtResult {
        CurrentBlock body_bb = ctx().make_current(body_block);

        auto result = body_bb.compile_loop_body(TIRO_NN(stmt->body()),
            TIRO_NN(stmt->body_scope()), end_block, cond_block);
        if (!result) {
            return result;
        };

        if (auto step = stmt->step()) {
            auto step_result = body_bb.compile_expr(
                TIRO_NN(step), ExprOptions::MaybeInvalid);
            if (!step_result)
                return step_result.failure();
        }

        body_bb.end(mir::Terminator::make_jump(cond_block));
        return ok;
    }();

    ctx().seal(end_block);
    ctx().seal(cond_block);
    bb().assign(end_block);
    return ok;
}

StmtResult StmtTransformer::visit_while_stmt(WhileStmt* stmt) {
    auto cond_block = ctx().make_block(strings().insert("while-cond"));
    auto body_block = ctx().make_block(strings().insert("while-body"));
    auto end_block = ctx().make_block(strings().insert("while-end"));
    bb().end(mir::Terminator::make_jump(cond_block));

    // Compile condition
    {
        CurrentBlock cond_bb = ctx().make_current(cond_block);
        auto cond_result = compile_loop_cond(
            stmt->condition(), body_block, end_block, cond_bb);
        if (!cond_result) {
            ctx().seal(cond_block);
            bb().assign(cond_block);
            return cond_result;
        }
    }
    ctx().seal(body_block);

    // Compile loop body.
    {
        CurrentBlock body_bb = ctx().make_current(body_block);
        auto body_result = body_bb.compile_loop_body(TIRO_NN(stmt->body()),
            TIRO_NN(stmt->body_scope()), end_block, cond_block);
        if (body_result) {
            body_bb.end(mir::Terminator::make_jump(cond_block));
        }
    }

    ctx().seal(end_block);
    ctx().seal(cond_block);
    bb().assign(end_block);
    return ok;
}

StmtResult StmtTransformer::compile_loop_cond(Expr* cond, mir::BlockID if_true,
    mir::BlockID if_false, CurrentBlock& cond_bb) {
    if (cond) {
        auto cond_result = cond_bb.compile_expr(TIRO_NN(cond));
        if (cond_result) {
            cond_bb.end(mir::Terminator::make_branch(
                mir::BranchType::IfFalse, *cond_result, if_false, if_true));
            return ok;
        }
        return cond_result.failure();
    }

    cond_bb.end(mir::Terminator::make_jump(if_true));
    return ok;
}

} // namespace tiro
