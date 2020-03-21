#ifndef TIRO_MIR_GEN_GEN_EXPR_HPP
#define TIRO_MIR_GEN_GEN_EXPR_HPP

#include "tiro/mir/fwd.hpp"
#include "tiro/mir_gen/gen_func.hpp"

namespace tiro {

class ExprMIRGen final : public Transformer {
public:
    ExprMIRGen(FunctionMIRGen& ctx, CurrentBlock& bb);

    ExprResult dispatch(NotNull<Expr*> expr);

    ExprResult visit_binary_expr(BinaryExpr* expr);
    ExprResult visit_block_expr(BlockExpr* expr);
    ExprResult visit_break_expr(BreakExpr* expr);
    ExprResult visit_call_expr(CallExpr* expr);
    ExprResult visit_continue_expr(ContinueExpr* expr);
    ExprResult visit_dot_expr(DotExpr* expr);
    ExprResult visit_if_expr(IfExpr* expr);
    ExprResult visit_index_expr(IndexExpr* expr);
    ExprResult visit_interpolated_string_expr(InterpolatedStringExpr* expr);
    ExprResult visit_array_literal(ArrayLiteral* expr);
    ExprResult visit_boolean_literal(BooleanLiteral* expr);
    ExprResult visit_float_literal(FloatLiteral* expr);
    ExprResult visit_func_literal(FuncLiteral* expr);
    ExprResult visit_integer_literal(IntegerLiteral* expr);
    ExprResult visit_map_literal(MapLiteral* expr);
    ExprResult visit_null_literal(NullLiteral* expr);
    ExprResult visit_set_literal(SetLiteral* expr);
    ExprResult visit_string_literal(StringLiteral* expr);
    ExprResult visit_symbol_literal(SymbolLiteral* expr);
    ExprResult visit_tuple_literal(TupleLiteral* expr);
    ExprResult visit_tuple_member_expr(TupleMemberExpr* expr);
    ExprResult visit_return_expr(ReturnExpr* expr);
    ExprResult visit_string_sequence_expr(StringSequenceExpr* expr);
    ExprResult visit_unary_expr(UnaryExpr* expr);
    ExprResult visit_var_expr(VarExpr* expr);

private:
    enum class LogicalOp { And, Or };

    ExprResult compile_assign(NotNull<Expr*> lhs, NotNull<Expr*> rhs);

    ExprResult compile_or(NotNull<Expr*> lhs, NotNull<Expr*> rhs);
    ExprResult compile_and(NotNull<Expr*> lhs, NotNull<Expr*> rhs);

    ExprResult
    compile_logical_op(LogicalOp op, NotNull<Expr*> lhs, NotNull<Expr*> rhs);

    TransformResult<LocalListID> compile_exprs(NotNull<ExprList*> args);

    BinaryOpType map_operator(BinaryOperator op);
    UnaryOpType map_operator(UnaryOperator op);
};

} // namespace tiro

#endif // TIRO_MIR_GEN_GEN_EXPR_HPP
