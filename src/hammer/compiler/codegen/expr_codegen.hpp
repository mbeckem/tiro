#ifndef HAMMER_COMPILER_CODEGEN_EXPR_CODEGEN_HPP
#define HAMMER_COMPILER_CODEGEN_EXPR_CODEGEN_HPP

#include "hammer/compiler/codegen/codegen.hpp"

namespace hammer::compiler {

/// This class is responsible for compiling expressions to bytecode.
class ExprCodegen final {
public:
    ExprCodegen(const NodePtr<Expr>& expr, FunctionCodegen& func);

    void generate();

    ModuleCodegen& module() { return func_.module(); }

public:
    void visit_unary_expr(const NodePtr<UnaryExpr>& e);
    void visit_binary_expr(const NodePtr<BinaryExpr>& e);
    void visit_var_expr(const NodePtr<VarExpr>& e);
    void visit_dot_expr(const NodePtr<DotExpr>& e);
    void visit_call_expr(const NodePtr<CallExpr>& e);
    void visit_index_expr(const NodePtr<IndexExpr>& e);
    void visit_if_expr(const NodePtr<IfExpr>& e);
    void visit_return_expr(const NodePtr<ReturnExpr>& e);
    void visit_continue_expr(const NodePtr<ContinueExpr>& e);
    void visit_break_expr(const NodePtr<BreakExpr>& e);
    void visit_block_expr(const NodePtr<BlockExpr>& e);
    void visit_string_sequence_expr(const NodePtr<StringSequenceExpr>& e);
    void visit_null_literal(const NodePtr<NullLiteral>& e);
    void visit_boolean_literal(const NodePtr<BooleanLiteral>& e);
    void visit_integer_literal(const NodePtr<IntegerLiteral>& e);
    void visit_float_literal(const NodePtr<FloatLiteral>& e);
    void visit_string_literal(const NodePtr<StringLiteral>& e);
    void visit_symbol_literal(const NodePtr<SymbolLiteral>& e);
    void visit_array_literal(const NodePtr<ArrayLiteral>& e);
    void visit_tuple_literal(const NodePtr<TupleLiteral>& e);
    void visit_map_literal(const NodePtr<MapLiteral>& e);
    void visit_set_literal(const NodePtr<SetLiteral>& e);
    void visit_func_literal(const NodePtr<FuncLiteral>& e);

private:
    void gen_assign(const NodePtr<BinaryExpr>& assign);
    void gen_member_assign(
        const NodePtr<DotExpr>& lhs, const NodePtr<Expr>& rhs, bool push_value);
    void gen_index_assign(const NodePtr<IndexExpr>& lhs,
        const NodePtr<Expr>& rhs, bool push_value);

    void gen_logical_and(const NodePtr<Expr>& lhs, const NodePtr<Expr>& rhs);
    void gen_logical_or(const NodePtr<Expr>& lhs, const NodePtr<Expr>& rhs);

private:
    const NodePtr<Expr>& expr_;
    FunctionCodegen& func_;
    CodeBuilder& builder_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_CODEGEN_EXPR_CODEGEN_HPP
