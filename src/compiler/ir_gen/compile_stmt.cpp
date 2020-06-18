#include "compiler/ir_gen/compile.hpp"

#include "compiler/ast/ast.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/semantics/symbol_table.hpp"

namespace tiro {

namespace {

class StmtCompiler final : private Transformer {
public:
    StmtCompiler(FunctionIRGen& ctx);

    OkResult dispatch(NotNull<AstStmt*> stmt, CurrentBlock& bb);

    OkResult visit_assert_stmt(NotNull<AstAssertStmt*> stmt, CurrentBlock& bb);
    OkResult visit_defer_stmt(NotNull<AstDeferStmt*> stmt, CurrentBlock& bb);
    OkResult visit_empty_stmt(NotNull<AstEmptyStmt*> stmt, CurrentBlock& bb);
    OkResult visit_expr_stmt(NotNull<AstExprStmt*> stmt, CurrentBlock& bb);
    OkResult visit_for_stmt(NotNull<AstForStmt*> stmt, CurrentBlock& bb);
    OkResult visit_decl_stmt(NotNull<AstDeclStmt*> stmt, CurrentBlock& bb);
    OkResult visit_while_stmt(NotNull<AstWhileStmt*> stmt, CurrentBlock& bb);

private:
    OkResult
    compile_loop_cond(AstExpr* cond, BlockId if_true, BlockId if_false, CurrentBlock& cond_bb);
};

} // namespace

StmtCompiler::StmtCompiler(FunctionIRGen& ctx)
    : Transformer(ctx) {}

OkResult StmtCompiler::dispatch(NotNull<AstStmt*> stmt, CurrentBlock& bb) {
    TIRO_DEBUG_ASSERT(
        !stmt->has_error(), "Nodes with errors must not reach the ir transformation stage.");
    return visit(stmt, *this, bb);
}

OkResult StmtCompiler::visit_assert_stmt(NotNull<AstAssertStmt*> stmt, CurrentBlock& bb) {
    auto cond_result = bb.compile_expr(TIRO_NN(stmt->cond()));
    if (!cond_result)
        return cond_result.failure();

    auto ok_block = ctx().make_block(strings().insert("assert-ok"));
    auto fail_block = ctx().make_block(strings().insert("assert-fail"));
    bb.end(Terminator::make_branch(BranchType::IfTrue, *cond_result, ok_block, fail_block));
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

    bb.assign(ok_block);
    return ok;
}

OkResult
StmtCompiler::visit_defer_stmt(NotNull<AstDeferStmt*> stmt, [[maybe_unused]] CurrentBlock& bb) {
    auto expr = TIRO_NN(stmt->expr());
    auto& scope = ctx().current_scope()->as_scope();
    TIRO_DEBUG_ASSERT(scope.processed == 0,
        "Cannot add additional deferred items when generating scope exit code.");
    scope.deferred.push_back(expr);
    return ok;
}

OkResult StmtCompiler::visit_empty_stmt(
    [[maybe_unused]] NotNull<AstEmptyStmt*> stmt, [[maybe_unused]] CurrentBlock& bb) {
    return ok;
}

OkResult StmtCompiler::visit_expr_stmt(NotNull<AstExprStmt*> stmt, CurrentBlock& bb) {
    auto result = bb.compile_expr(TIRO_NN(stmt->expr()), ExprOptions::MaybeInvalid);
    if (!result)
        return result.failure();

    return ok;
}

OkResult StmtCompiler::visit_for_stmt(NotNull<AstForStmt*> stmt, CurrentBlock& bb) {
    if (auto decl = stmt->decl()) {
        auto decl_result = compile_var_decl(TIRO_NN(decl), bb);
        if (!decl_result)
            return decl_result;
    }

    auto cond_block = ctx().make_block(strings().insert("for-cond"));
    auto body_block = ctx().make_block(strings().insert("for-body"));
    auto step_block = ctx().make_block(strings().insert("for-step"));
    auto end_block = ctx().make_block(strings().insert("for-end"));
    bb.end(Terminator::make_jump(cond_block));

    // Compile condition.
    auto cond_result = [&]() -> OkResult {
        CurrentBlock cond_bb = ctx().make_current(cond_block);
        return compile_loop_cond(stmt->cond(), body_block, end_block, cond_bb);
    }();

    if (cond_result) {
        // Compile loop body.
        // Condition is the only item that jumps to the body block.
        ctx().seal(body_block);
        [[maybe_unused]] auto body_result = [&]() -> OkResult {
            CurrentBlock body_bb = ctx().make_current(body_block);
            auto result = body_bb.compile_loop_body(TIRO_NN(stmt->body()), end_block, step_block);
            if (!result)
                return result;

            body_bb.end(Terminator::make_jump(step_block));
            return ok;
        }();

        // Compile step function.
        // Body block is the only item that jumps to the step block (possibly using "continue").
        ctx().seal(step_block);
        [[maybe_unused]] auto step_result = [&]() -> OkResult {
            CurrentBlock step_bb = ctx().make_current(step_block);

            if (ctx().result()[step_block]->predecessor_count() == 0) {
                step_bb.end(Terminator::make_never(ctx().result().exit()));
                return unreachable;
            }

            if (auto step = stmt->step()) {
                auto result = step_bb.compile_expr(TIRO_NN(step), ExprOptions::MaybeInvalid);
                if (!result)
                    return result.failure();
            }

            step_bb.end(Terminator::make_jump(cond_block));
            return ok;
        }();
    }

    ctx().seal(cond_block);
    ctx().seal(end_block);
    bb.assign(end_block);
    return cond_result;
}

OkResult StmtCompiler::visit_decl_stmt(NotNull<AstDeclStmt*> stmt, CurrentBlock& bb) {
    auto decl = TIRO_NN(stmt->decl());

    switch (decl->type()) {
    case AstNodeType::VarDecl:
        return compile_var_decl(must_cast<AstVarDecl>(decl), bb);

    case AstNodeType::FuncDecl:
    case AstNodeType::ImportDecl:
    case AstNodeType::ParamDecl:
    default:
        TIRO_ERROR("Invalid declaration type in this context: {}.", decl->type());
    }
}

OkResult StmtCompiler::visit_while_stmt(NotNull<AstWhileStmt*> stmt, CurrentBlock& bb) {
    auto cond_block = ctx().make_block(strings().insert("while-cond"));
    auto body_block = ctx().make_block(strings().insert("while-body"));
    auto end_block = ctx().make_block(strings().insert("while-end"));
    bb.end(Terminator::make_jump(cond_block));

    // Compile condition
    {
        CurrentBlock cond_bb = ctx().make_current(cond_block);
        auto cond_result = compile_loop_cond(stmt->cond(), body_block, end_block, cond_bb);
        if (!cond_result) {
            ctx().seal(cond_block);
            bb.assign(cond_block);
            return cond_result;
        }
    }

    // Compile loop body.
    ctx().seal(body_block);
    {
        CurrentBlock body_bb = ctx().make_current(body_block);
        auto body_result = body_bb.compile_loop_body(TIRO_NN(stmt->body()), end_block, cond_block);
        if (body_result) {
            body_bb.end(Terminator::make_jump(cond_block));
        }
    }

    ctx().seal(end_block);
    ctx().seal(cond_block);
    bb.assign(end_block);
    return ok;
}

OkResult StmtCompiler::compile_loop_cond(
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

OkResult compile_stmt(NotNull<AstStmt*> stmt, CurrentBlock& bb) {
    StmtCompiler gen(bb.ctx());
    return gen.dispatch(stmt, bb);
}

} // namespace tiro
