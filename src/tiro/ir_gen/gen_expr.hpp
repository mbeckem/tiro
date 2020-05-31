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
            outl(f"LocalResult {node_type.visitor_name}(NotNull<{node_type.cpp_name}*> expr);")
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

    // Compiles the simple binary operator, e.g. "a + b";
    LocalResult compile_binary(BinaryOpType op, NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);

    // Complies a simple assigmnet, e.g. "a = b", or "(a, b, c) = f".
    LocalResult compile_assign(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);

    // Compiles the compound assignment operator, e.g. "a += b";
    LocalResult
    compile_compound_assign(BinaryOpType op, NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);

    // Compiles the expression (which must represent a single left hand side value) and returns the target location.
    // This is being used to implement constructs such as "a = b" or "a.b = c".
    TransformResult<AssignTarget> compile_target(NotNull<AstExpr*> expr);

    // Compiles the given tuple literal expression as a set of assignment targets.
    // Used for tuple assignments such as "(a, b) = f()".
    TransformResult<std::vector<AssignTarget>>
    compile_tuple_targets(NotNull<AstTupleLiteral*> tuple);

    LocalResult compile_or(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);
    LocalResult compile_and(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);
    LocalResult compile_logical_op(LogicalOp op, NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs);

    template<typename ExprType>
    TransformResult<LocalListId> compile_exprs(const AstNodeList<ExprType>& args);

    ValueType get_type(NotNull<AstExpr*> expr) const;
    SymbolId get_symbol(NotNull<AstVarExpr*> expr) const;

    bool can_elide() const;

private:
    ExprOptions opts_;
};

} // namespace tiro

#endif // TIRO_IR_GEN_GEN_EXPR_HPP
