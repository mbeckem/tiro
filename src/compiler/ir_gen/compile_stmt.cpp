#include "compiler/ir_gen/compile.hpp"

#include "compiler/ast/ast.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/semantics/symbol_table.hpp"

namespace tiro::ir {

namespace {

class StmtCompiler final : private Transformer {
public:
    StmtCompiler(FunctionIRGen& ctx);

    OkResult dispatch(NotNull<AstStmt*> stmt, CurrentBlock& bb);

    OkResult visit_assert_stmt(NotNull<AstAssertStmt*> stmt, CurrentBlock& bb);
    OkResult visit_defer_stmt(NotNull<AstDeferStmt*> stmt, CurrentBlock& bb);
    OkResult visit_empty_stmt(NotNull<AstEmptyStmt*> stmt, CurrentBlock& bb);
    OkResult visit_error_stmt(NotNull<AstErrorStmt*> stmt, CurrentBlock& bb);
    OkResult visit_expr_stmt(NotNull<AstExprStmt*> stmt, CurrentBlock& bb);
    OkResult visit_for_stmt(NotNull<AstForStmt*> stmt, CurrentBlock& bb);
    OkResult visit_for_each_stmt(NotNull<AstForEachStmt*> stmt, CurrentBlock& bb);
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
        auto expr_string_view = substring(ctx().source_file(), stmt->cond()->full_range());
        auto expr_string = strings().insert(expr_string_view);
        auto expr_inst = nested.compile_value(Constant::make_string(expr_string));

        // The message expression is optional (but should evaluate to a string, if present).
        auto message_result = [&]() -> InstResult {
            if (stmt->message())
                return nested.compile_expr(TIRO_NN(stmt->message()));

            return nested.compile_value(Constant::make_null());
        }();
        if (!message_result)
            return message_result.failure();

        nested.end(Terminator::make_assert_fail(expr_inst, *message_result, result().exit()));
    }

    bb.assign(ok_block);
    return ok;
}

OkResult StmtCompiler::visit_defer_stmt(NotNull<AstDeferStmt*> stmt, CurrentBlock& bb) {
    auto expr = TIRO_NN(stmt->expr());

    // Abnormal (exceptional) control flow: the expression is compiled as a handler. All future basic blocks will
    // point to that handler (handler edge), until the scope exit or until another defer statement is encountered.
    // NOTE: the new handler block inherits the current exception handler from the ctx.
    auto handler_block = ctx().make_handler_block(strings().insert("defer-panic"));
    [&] {
        CurrentBlock nested = ctx().make_current(handler_block);
        auto expr_result = nested.compile_expr(expr, ExprOptions::MaybeInvalid);
        if (!expr_result)
            return;

        nested.end(Terminator::make_rethrow(ctx().result().exit()));
    }();

    // Normal control flow: the expression is remembered and compiled by the scope exit (ExprCompiler::visit_block_expr).
    {
        auto& scope = ctx().current_scope()->as_scope();
        TIRO_DEBUG_ASSERT(scope.processed == 0,
            "Cannot add additional deferred items when generating scope exit code.");
        scope.deferred.emplace_back(expr, ctx().current_handler());
    }

    // Register the handler block as the exception handler for all new blocks. Scope exit
    // will clean this up.
    ctx().current_handler(handler_block);
    bb.advance(strings().insert("defer-continue"));
    return ok;
}

OkResult StmtCompiler::visit_empty_stmt(
    [[maybe_unused]] NotNull<AstEmptyStmt*> stmt, [[maybe_unused]] CurrentBlock& bb) {
    return ok;
}

OkResult StmtCompiler::visit_error_stmt(NotNull<AstErrorStmt*>, CurrentBlock&) {
    TIRO_ERROR("Attempt to compile an invalid AST.");
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
        [&]() {
            CurrentBlock body_bb = ctx().make_current(body_block);

            auto body = TIRO_NN(stmt->body());
            auto body_scope_id = symbols().get_scope(body->id());
            auto body_result = body_bb.compile_loop_body(
                body_scope_id,
                [&]() -> OkResult {
                    auto result = body_bb.compile_expr(body, ExprOptions::MaybeInvalid);
                    if (!result)
                        return result.failure();
                    return ok;
                },
                end_block, step_block);

            if (body_result)
                body_bb.end(Terminator::make_jump(step_block));
        }();

        // Compile step function.
        // Body block is the only item that jumps to the step block (possibly using "continue").
        ctx().seal(step_block);
        [&]() {
            CurrentBlock step_bb = ctx().make_current(step_block);

            if (ctx().result()[step_block]->predecessor_count() == 0) {
                step_bb.end(Terminator::make_never(ctx().result().exit()));
                return;
            }

            if (auto step = stmt->step()) {
                auto result = step_bb.compile_expr(TIRO_NN(step), ExprOptions::MaybeInvalid);
                if (!result)
                    return;
            }

            step_bb.end(Terminator::make_jump(cond_block));
        }();
    }

    ctx().seal(cond_block);
    ctx().seal(end_block);
    bb.assign(end_block);
    return cond_result;
}

OkResult StmtCompiler::visit_for_each_stmt(NotNull<AstForEachStmt*> stmt, CurrentBlock& bb) {
    // Compile iterator creation.
    auto container_result = bb.compile_expr(TIRO_NN(stmt->expr()));
    if (!container_result)
        return container_result.failure();
    auto iterator = bb.compile_value(Value::make_make_iterator(*container_result));

    auto step_block = ctx().make_block(strings().insert("for-each-step"));
    auto body_block = ctx().make_block(strings().insert("for-each-body"));
    auto end_block = ctx().make_block(strings().insert("for-each-end"));
    bb.end(Terminator::make_jump(step_block));

    // Compile iterator advance.
    auto iter_next = [&]() {
        CurrentBlock step_bb = ctx().make_current(step_block);
        auto next = step_bb.compile_value(Aggregate::make_iterator_next(iterator));
        auto valid = step_bb.compile_value(
            Value::make_get_aggregate_member(next, AggregateMember::IteratorNextValid));

        step_bb.end(Terminator::make_branch(BranchType::IfFalse, valid, end_block, body_block));
        return next;
    }();

    // Compile loop body
    ctx().seal(body_block);
    {
        CurrentBlock body_bb = ctx().make_current(body_block);

        auto spec = TIRO_NN(stmt->spec());
        auto body = TIRO_NN(stmt->body());

        auto body_scope_id = symbols().get_scope(stmt->id());
        auto body_result = body_bb.compile_loop_body(
            body_scope_id,
            [&]() -> OkResult {
                auto value = body_bb.compile_value(Value::make_get_aggregate_member(
                    iter_next, AggregateMember::IteratorNextValue));
                compile_spec_assign(spec, value, body_bb);

                auto result = body_bb.compile_expr(body, ExprOptions::MaybeInvalid);
                if (!result)
                    return result.failure();

                return ok;
            },
            end_block, step_block);

        if (body_result)
            body_bb.end(Terminator::make_jump(step_block));
    }

    ctx().seal(step_block);
    ctx().seal(end_block);
    bb.assign(end_block);
    return ok;
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

        auto body = TIRO_NN(stmt->body());
        auto body_scope_id = symbols().get_scope(body->id());
        auto body_result = body_bb.compile_loop_body(
            body_scope_id,
            [&]() -> OkResult {
                auto result = body_bb.compile_expr(body, ExprOptions::MaybeInvalid);
                if (!result)
                    return result.failure();
                return ok;
            },
            end_block, cond_block);

        if (body_result)
            body_bb.end(Terminator::make_jump(cond_block));
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

} // namespace tiro::ir
