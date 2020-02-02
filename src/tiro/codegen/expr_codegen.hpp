#ifndef TIRO_CODEGEN_EXPR_CODEGEN_HPP
#define TIRO_CODEGEN_EXPR_CODEGEN_HPP

#include "tiro/codegen/basic_block.hpp"
#include "tiro/codegen/code_builder.hpp"
#include "tiro/core/function_ref.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/syntax/ast.hpp"

namespace tiro::compiler {

class FunctionCodegen;
class ModuleCodegen;

/// This class is responsible for compiling expressions to bytecode.
class ExprCodegen final {
public:
    explicit ExprCodegen(
        NotNull<Expr*> expr, CurrentBasicBlock& bb, FunctionCodegen& func);

    /// Returns false if value generation was omitted as an optimization.
    bool generate();

    ModuleCodegen& module();

public:
    bool visit_unary_expr(UnaryExpr* e);
    bool visit_binary_expr(BinaryExpr* e);
    bool visit_var_expr(VarExpr* e);
    bool visit_dot_expr(DotExpr* e);
    bool visit_tuple_member_expr(TupleMemberExpr* e);
    bool visit_call_expr(CallExpr* e);
    bool visit_index_expr(IndexExpr* e);
    bool visit_if_expr(IfExpr* e);
    bool visit_return_expr(ReturnExpr* e);
    bool visit_continue_expr(ContinueExpr* e);
    bool visit_break_expr(BreakExpr* e);
    bool visit_block_expr(BlockExpr* e);
    bool visit_string_sequence_expr(StringSequenceExpr* e);
    bool visit_interpolated_string_expr(InterpolatedStringExpr* e);
    bool visit_null_literal(NullLiteral* e);
    bool visit_boolean_literal(BooleanLiteral* e);
    bool visit_integer_literal(IntegerLiteral* e);
    bool visit_float_literal(FloatLiteral* e);
    bool visit_string_literal(StringLiteral* e);
    bool visit_symbol_literal(SymbolLiteral* e);
    bool visit_array_literal(ArrayLiteral* e);
    bool visit_tuple_literal(TupleLiteral* e);
    bool visit_map_literal(MapLiteral* e);
    bool visit_set_literal(SetLiteral* e);
    bool visit_func_literal(FuncLiteral* e);

private:
    bool gen_assign(NotNull<BinaryExpr*> assign);

    void gen_store(NotNull<Expr*> lhs, NotNull<Expr*> rhs, bool has_value);

    void
    gen_var_store(NotNull<VarExpr*> lhs, NotNull<Expr*> rhs, bool has_value);

    void
    gen_member_store(NotNull<DotExpr*> lhs, NotNull<Expr*> rhs, bool has_value);

    void gen_tuple_member_store(
        NotNull<TupleMemberExpr*> lhs, NotNull<Expr*> rhs, bool has_value);

    void gen_index_store(
        NotNull<IndexExpr*> lhs, NotNull<Expr*> rhs, bool has_value);

    void gen_tuple_store(
        NotNull<TupleLiteral*> lhs, NotNull<Expr*> rhs, bool has_value);

    void gen_logical_and(NotNull<Expr*> lhs, NotNull<Expr*> rhs);
    void gen_logical_or(NotNull<Expr*> lhs, NotNull<Expr*> rhs);

    void gen_loop_jump(NotNull<BasicBlock*> target);

private:
    Expr* expr_;
    FunctionCodegen& func_;
    CurrentBasicBlock& bb_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace tiro::compiler

#endif // TIRO_CODEGEN_EXPR_CODEGEN_HPP
