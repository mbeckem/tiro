#ifndef TIRO_IR_GEN_GEN_EXPR_HPP
#define TIRO_IR_GEN_GEN_EXPR_HPP

#include "tiro/ir/fwd.hpp"
#include "tiro/ir_gen/gen_func.hpp"

namespace tiro {

class ExprIRGen final : public Transformer {
public:
    ExprIRGen(FunctionIRGen& ctx, ExprOptions opts, CurrentBlock& bb);

    LocalResult dispatch(NotNull<AstExpr*> expr);

    /* [[[cog
        from cog import outl
        from codegen.ast import NODE_TYPES, walk_concrete_types
        for node_type in walk_concrete_types(NODE_TYPES.get("Expr")):
            outl(f"ExprResult {node_type.visitor_name}(NotNull<{node_type.cpp_name}*> expr);")
    ]]] */
    LocalResult visit_binary_expr(NotNull<AstBinaryExpr*> expr);
    LocalResult visit_block_expr(NotNull<AstBlockExpr*> expr);
    LocalResult visit_break_expr(NotNull<AstBreakExpr*> expr);
    LocalResult visit_call_expr(NotNull<AstCallExpr*> expr);
    LocalResult visit_continue_expr(NotNull<AstContinueExpr*> expr);
    LocalResult visit_element_expr(NotNull<AstElementExpr*> expr);
    LocalResult visit_func_expr(NotNull<AstFuncExpr*> expr);
    LocalResult visit_if_expr(NotNull<AstIfExpr*> expr);
    LocalResult visit_array_literal(NotNull<AstArrayLiteral*> expr);
    LocalResult visit_boolean_literal(NotNull<AstBooleanLiteral*> expr);
    LocalResult visit_float_literal(NotNull<AstFloatLiteral*> expr);
    LocalResult visit_integer_literal(NotNull<AstIntegerLiteral*> expr);
    LocalResult visit_map_literal(NotNull<AstMapLiteral*> expr);
    LocalResult visit_null_literal(NotNull<AstNullLiteral*> expr);
    LocalResult visit_set_literal(NotNull<AstSetLiteral*> expr);
    LocalResult visit_string_literal(NotNull<AstStringLiteral*> expr);
    LocalResult visit_symbol_literal(NotNull<AstSymbolLiteral*> expr);
    LocalResult visit_tuple_literal(NotNull<AstTupleLiteral*> expr);
    LocalResult visit_property_expr(NotNull<AstPropertyExpr*> expr);
    LocalResult visit_return_expr(NotNull<AstReturnExpr*> expr);
    LocalResult visit_string_expr(NotNull<AstStringExpr*> expr);
    LocalResult visit_string_group_expr(NotNull<AstStringGroupExpr*> expr);
    LocalResult visit_unary_expr(NotNull<AstUnaryExpr*> expr);
    LocalResult visit_var_expr(NotNull<AstVarExpr*> expr);
    // [[[end]]]

private:
    enum class LogicalOp { And, Or };

    LocalResult compile_assign(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);

    LocalResult compile_or(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);
    LocalResult compile_and(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);

    LocalResult compile_logical_op(LogicalOp op, NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);

    template<typename ExprType>
    TransformResult<LocalListId> compile_exprs(const AstNodeList<ExprType>& args);

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
