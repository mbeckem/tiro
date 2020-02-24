#ifndef TIRO_MIR_TRANSFORM_STMT_HPP
#define TIRO_MIR_TRANSFORM_STMT_HPP

#include "tiro/mir/fwd.hpp"
#include "tiro/mir/transform_func.hpp"

namespace tiro::compiler::mir_transform {

class StmtTransformer final : private Transformer {
public:
    StmtTransformer(FunctionContext& ctx, CurrentBlock& bb);

    StmtResult dispatch(NotNull<Stmt*> stmt);

    StmtResult visit_assert_stmt(AssertStmt* stmt);
    StmtResult visit_decl_stmt(DeclStmt* stmt);
    StmtResult visit_empty_stmt(EmptyStmt* stmt);
    StmtResult visit_expr_stmt(ExprStmt* stmt);
    StmtResult visit_for_stmt(ForStmt* stmt);
    StmtResult visit_while_stmt(WhileStmt* stmt);

private:
    StmtResult compile_loop_cond(Expr* cond, mir::BlockID if_true,
        mir::BlockID if_false, CurrentBlock& cond_bb);
};

} // namespace tiro::compiler::mir_transform

#endif // TIRO_MIR_TRANSFORM_STMT_HPP
