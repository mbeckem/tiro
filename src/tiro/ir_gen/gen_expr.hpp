#ifndef TIRO_IR_GEN_GEN_EXPR_HPP
#define TIRO_IR_GEN_GEN_EXPR_HPP

#include "tiro/ir/fwd.hpp"
#include "tiro/ir_gen/gen_func.hpp"

namespace tiro {

class ExprIRGen final : public Transformer {
public:
    ExprIRGen(FunctionIRGen& ctx, ExprOptions opts, CurrentBlock& bb);

    ExprResult dispatch(NotNull<AstExpr*> expr);

    /* [[[cog
        from cog import outl
        from codegen.ast import NODE_TYPES, walk_concrete_types
        for node_type in walk_concrete_types(NODE_TYPES.get("Expr")):
            outl(f"ExprResult {node_type.visitor_name}(NotNull<{node_type.cpp_name}*> expr);")
    ]]] */
    ExprResult visit_binary_expr(NotNull<AstBinaryExpr*> expr);
    ExprResult visit_block_expr(NotNull<AstBlockExpr*> expr);
    ExprResult visit_break_expr(NotNull<AstBreakExpr*> expr);
    ExprResult visit_call_expr(NotNull<AstCallExpr*> expr);
    ExprResult visit_continue_expr(NotNull<AstContinueExpr*> expr);
    ExprResult visit_element_expr(NotNull<AstElementExpr*> expr);
    ExprResult visit_func_expr(NotNull<AstFuncExpr*> expr);
    ExprResult visit_if_expr(NotNull<AstIfExpr*> expr);
    ExprResult visit_array_literal(NotNull<AstArrayLiteral*> expr);
    ExprResult visit_boolean_literal(NotNull<AstBooleanLiteral*> expr);
    ExprResult visit_float_literal(NotNull<AstFloatLiteral*> expr);
    ExprResult visit_integer_literal(NotNull<AstIntegerLiteral*> expr);
    ExprResult visit_map_literal(NotNull<AstMapLiteral*> expr);
    ExprResult visit_null_literal(NotNull<AstNullLiteral*> expr);
    ExprResult visit_set_literal(NotNull<AstSetLiteral*> expr);
    ExprResult visit_string_literal(NotNull<AstStringLiteral*> expr);
    ExprResult visit_symbol_literal(NotNull<AstSymbolLiteral*> expr);
    ExprResult visit_tuple_literal(NotNull<AstTupleLiteral*> expr);
    ExprResult visit_property_expr(NotNull<AstPropertyExpr*> expr);
    ExprResult visit_return_expr(NotNull<AstReturnExpr*> expr);
    ExprResult visit_string_expr(NotNull<AstStringExpr*> expr);
    ExprResult visit_string_group_expr(NotNull<AstStringGroupExpr*> expr);
    ExprResult visit_unary_expr(NotNull<AstUnaryExpr*> expr);
    ExprResult visit_var_expr(NotNull<AstVarExpr*> expr);
    // [[[end]]]

private:
    enum class LogicalOp { And, Or };

    ExprResult compile_assign(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);

    ExprResult compile_or(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);
    ExprResult compile_and(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);

    ExprResult compile_logical_op(
        LogicalOp op, NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);

    template<typename ExprType>
    TransformResult<LocalListId>
    compile_exprs(const AstNodeList<ExprType>& args);

    BinaryOpType map_operator(BinaryOperator op);
    UnaryOpType map_operator(UnaryOperator op);

    ValueType get_type(NotNull<AstExpr*> expr) const;
    SymbolId get_symbol(NotNull<AstVarExpr*> expr) const;

    bool can_elide() const;

private:
    ExprOptions opts_;
};

} // namespace tiro

#endif // TIRO_IR_GEN_GEN_EXPR_HPP
