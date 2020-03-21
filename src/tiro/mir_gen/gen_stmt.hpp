#ifndef TIRO_MIR_GEN_GEN_STMT_HPP
#define TIRO_MIR_GEN_GEN_STMT_HPP

#include "tiro/mir/fwd.hpp"
#include "tiro/mir_gen/gen_func.hpp"

namespace tiro {

class StmtMIRGen final : private Transformer {
public:
    StmtMIRGen(FunctionMIRGen& ctx, CurrentBlock& bb);

    StmtResult dispatch(NotNull<ASTStmt*> stmt);

    StmtResult visit_assert_stmt(AssertStmt* stmt);
    StmtResult visit_decl_stmt(DeclStmt* stmt);
    StmtResult visit_empty_stmt(EmptyStmt* stmt);
    StmtResult visit_expr_stmt(ExprStmt* stmt);
    StmtResult visit_for_stmt(ForStmt* stmt);
    StmtResult visit_while_stmt(WhileStmt* stmt);

private:
    StmtResult compile_loop_cond(Expr* cond, BlockID if_true,
        BlockID if_false, CurrentBlock& cond_bb);
};

} // namespace tiro

#endif // TIRO_MIR_GEN_GEN_STMT_HPP
