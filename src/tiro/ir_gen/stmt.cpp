#include "tiro/ir_gen/stmt.hpp"

#include "tiro/ast/ast.hpp"
#include "tiro/ir/function.hpp"
#include "tiro/ir_gen/assign.hpp"
#include "tiro/semantics/symbol_table.hpp"

namespace tiro {

StmtIRGen::StmtIRGen(FunctionIRGen& ctx, CurrentBlock& bb)
    : Transformer(ctx, bb) {}

OkResult StmtIRGen::dispatch(NotNull<AstStmt*> stmt) {
    TIRO_DEBUG_ASSERT(
        !stmt->has_error(), "Nodes with errors must not reach the ir transformation stage.");
    return visit(stmt, *this);
}

OkResult StmtIRGen::visit_assert_stmt(NotNull<AstAssertStmt*> stmt) {
    auto cond_result = bb().compile_expr(TIRO_NN(stmt->cond()));
    if (!cond_result)
        return cond_result.failure();

    auto ok_block = ctx().make_block(strings().insert("assert-ok"));
    auto fail_block = ctx().make_block(strings().insert("assert-fail"));
    bb().end(Terminator::make_branch(BranchType::IfTrue, *cond_result, ok_block, fail_block));
    ctx().seal(fail_block);
    ctx().seal(ok_block);

    {
        auto nested = ctx().make_current(fail_block);

        // The expression (in source code form) that failed to return true.
        // TODO: Take the expression from the source code
        auto expr_string = strings().insert("expression");
        auto expr_local = nested.compile_rvalue(Constant::make_string(expr_string));

        // The message expression is optional (but should evaluate to a string, if present).
        auto message_result = [&]() -> LocalResult {
            if (stmt->message())
                return nested.compile_expr(TIRO_NN(stmt->message()));

            return nested.compile_rvalue(Constant::make_null());
        }();
        if (!message_result)
            return message_result.failure();

        nested.end(Terminator::make_assert_fail(expr_local, *message_result, result().exit()));
    }

    bb().assign(ok_block);
    return ok;
}

OkResult StmtIRGen::visit_empty_stmt([[maybe_unused]] NotNull<AstEmptyStmt*> stmt) {
    return ok;
}

OkResult StmtIRGen::visit_expr_stmt(NotNull<AstExprStmt*> stmt) {
    auto result = bb().compile_expr(TIRO_NN(stmt->expr()), ExprOptions::MaybeInvalid);
    if (!result)
        return result.failure();

    return ok;
}

OkResult StmtIRGen::visit_for_stmt(NotNull<AstForStmt*> stmt) {
    if (auto decl = stmt->decl()) {
        auto decl_result = compile_var_decl(TIRO_NN(decl), bb());
        if (!decl_result)
            return decl_result;
    }

    auto cond_block = ctx().make_block(strings().insert("for-cond"));
    auto body_block = ctx().make_block(strings().insert("for-body"));
    auto end_block = ctx().make_block(strings().insert("for-end"));
    bb().end(Terminator::make_jump(cond_block));

    // Compile condition.
    {
        CurrentBlock cond_bb = ctx().make_current(cond_block);
        auto cond_result = compile_loop_cond(stmt->cond(), body_block, end_block, cond_bb);
        if (!cond_result) {
            ctx().seal(cond_block);
            bb().assign(cond_block);
            return cond_result;
        }
    }
    ctx().seal(body_block);

    // Compile loop body.
    [[maybe_unused]] auto body_result = [&]() -> OkResult {
        CurrentBlock body_bb = ctx().make_current(body_block);

        auto result = body_bb.compile_loop_body(TIRO_NN(stmt->body()), end_block, cond_block);
        if (!result) {
            return result;
        };

        if (auto step = stmt->step()) {
            auto step_result = body_bb.compile_expr(TIRO_NN(step), ExprOptions::MaybeInvalid);
            if (!step_result)
                return step_result.failure();
        }

        body_bb.end(Terminator::make_jump(cond_block));
        return ok;
    }();

    ctx().seal(end_block);
    ctx().seal(cond_block);
    bb().assign(end_block);
    return ok;
}

OkResult StmtIRGen::visit_var_stmt(NotNull<AstVarStmt*> stmt) {
    return compile_var_decl(TIRO_NN(stmt->decl()), bb());
}

OkResult StmtIRGen::visit_while_stmt(NotNull<AstWhileStmt*> stmt) {
    auto cond_block = ctx().make_block(strings().insert("while-cond"));
    auto body_block = ctx().make_block(strings().insert("while-body"));
    auto end_block = ctx().make_block(strings().insert("while-end"));
    bb().end(Terminator::make_jump(cond_block));

    // Compile condition
    {
        CurrentBlock cond_bb = ctx().make_current(cond_block);
        auto cond_result = compile_loop_cond(stmt->cond(), body_block, end_block, cond_bb);
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
        auto body_result = body_bb.compile_loop_body(TIRO_NN(stmt->body()), end_block, cond_block);
        if (body_result) {
            body_bb.end(Terminator::make_jump(cond_block));
        }
    }

    ctx().seal(end_block);
    ctx().seal(cond_block);
    bb().assign(end_block);
    return ok;
}

OkResult StmtIRGen::compile_loop_cond(
    AstExpr* cond, BlockId if_true, BlockId if_false, CurrentBlock& cond_bb) {
    if (cond) {
        auto cond_result = cond_bb.compile_expr(TIRO_NN(cond));
        if (cond_result) {
            cond_bb.end(
                Terminator::make_branch(BranchType::IfFalse, *cond_result, if_false, if_true));
            return ok;
        }
        return cond_result.failure();
    }

    cond_bb.end(Terminator::make_jump(if_true));
    return ok;
}

} // namespace tiro
