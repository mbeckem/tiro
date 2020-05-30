#ifndef TIRO_IR_GEN_GEN_STMT_HPP
#define TIRO_IR_GEN_GEN_STMT_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/ir/fwd.hpp"
#include "tiro/ir_gen/gen_func.hpp"

namespace tiro {

class StmtIRGen final : private Transformer {
public:
    StmtIRGen(FunctionIRGen& ctx, CurrentBlock& bb);

    OkResult dispatch(NotNull<AstStmt*> stmt);

    OkResult visit_assert_stmt(NotNull<AstAssertStmt*> stmt);
    OkResult visit_empty_stmt(NotNull<AstEmptyStmt*> stmt);
    OkResult visit_expr_stmt(NotNull<AstExprStmt*> stmt);
    OkResult visit_for_stmt(NotNull<AstForStmt*> stmt);
    OkResult visit_var_stmt(NotNull<AstVarStmt*> stmt);
    OkResult visit_while_stmt(NotNull<AstWhileStmt*> stmt);

private:
    OkResult compile_loop_cond(AstExpr* cond, BlockId if_true, BlockId if_false,
        CurrentBlock& cond_bb);
};

} // namespace tiro

#endif // TIRO_IR_GEN_GEN_STMT_HPP
