#ifndef TIRO_CODEGEN_EXPR_CODEGEN_HPP
#define TIRO_CODEGEN_EXPR_CODEGEN_HPP

#include "tiro/codegen/codegen.hpp"
#include "tiro/core/function_ref.hpp"

namespace tiro::compiler {

/// This class is responsible for compiling expressions to bytecode.
class ExprCodegen final {
public:
    ExprCodegen(Expr* expr, FunctionCodegen& func);

    /// Returns false if value generation was omitted as an optimization.
    bool generate();

    ModuleCodegen& module() { return func_.module(); }

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
    bool gen_assign(BinaryExpr* assign);

    void gen_store(Expr* lhs, Expr* rhs, bool has_value);
    void gen_var_store(VarExpr* lhs, Expr* rhs, bool has_value);
    void gen_member_store(DotExpr* lhs, Expr* rhs, bool has_value);
    void
    gen_tuple_member_store(TupleMemberExpr* lhs, Expr* rhs, bool has_value);
    void gen_index_store(IndexExpr* lhs, Expr* rhs, bool has_value);
    void gen_tuple_store(TupleLiteral* lhs, Expr* rhs, bool has_value);

    void gen_logical_and(Expr* lhs, Expr* rhs);
    void gen_logical_or(Expr* lhs, Expr* rhs);

private:
    Expr* expr_;
    FunctionCodegen& func_;
    CodeBuilder& builder_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace tiro::compiler

#endif // TIRO_CODEGEN_EXPR_CODEGEN_HPP
