#ifndef HAMMER_COMPILER_CODEGEN_EXPR_CODEGEN_HPP
#define HAMMER_COMPILER_CODEGEN_EXPR_CODEGEN_HPP

#include "hammer/compiler/codegen/codegen.hpp"

namespace hammer::compiler {

/// This class is responsible for compiling expressions to bytecode.
class ExprCodegen final {
public:
    ExprCodegen(const NodePtr<Expr>& expr, FunctionCodegen& func);

    /// Returns false if value generation was omitted as an optimization.
    bool generate();

    ModuleCodegen& module() { return func_.module(); }

public:
    bool visit_unary_expr(const NodePtr<UnaryExpr>& e);
    bool visit_binary_expr(const NodePtr<BinaryExpr>& e);
    bool visit_var_expr(const NodePtr<VarExpr>& e);
    bool visit_dot_expr(const NodePtr<DotExpr>& e);
    bool visit_tuple_member_expr(const NodePtr<TupleMemberExpr>& e);
    bool visit_call_expr(const NodePtr<CallExpr>& e);
    bool visit_index_expr(const NodePtr<IndexExpr>& e);
    bool visit_if_expr(const NodePtr<IfExpr>& e);
    bool visit_return_expr(const NodePtr<ReturnExpr>& e);
    bool visit_continue_expr(const NodePtr<ContinueExpr>& e);
    bool visit_break_expr(const NodePtr<BreakExpr>& e);
    bool visit_block_expr(const NodePtr<BlockExpr>& e);
    bool visit_string_sequence_expr(const NodePtr<StringSequenceExpr>& e);
    bool visit_null_literal(const NodePtr<NullLiteral>& e);
    bool visit_boolean_literal(const NodePtr<BooleanLiteral>& e);
    bool visit_integer_literal(const NodePtr<IntegerLiteral>& e);
    bool visit_float_literal(const NodePtr<FloatLiteral>& e);
    bool visit_string_literal(const NodePtr<StringLiteral>& e);
    bool visit_symbol_literal(const NodePtr<SymbolLiteral>& e);
    bool visit_array_literal(const NodePtr<ArrayLiteral>& e);
    bool visit_tuple_literal(const NodePtr<TupleLiteral>& e);
    bool visit_map_literal(const NodePtr<MapLiteral>& e);
    bool visit_set_literal(const NodePtr<SetLiteral>& e);
    bool visit_func_literal(const NodePtr<FuncLiteral>& e);

private:
    bool gen_assign(const NodePtr<BinaryExpr>& assign);
    void gen_member_assign(
        const NodePtr<DotExpr>& lhs, const NodePtr<Expr>& rhs, bool push_value);
    void gen_tuple_member_assign(const NodePtr<TupleMemberExpr>& lhs,
        const NodePtr<Expr>& rhs, bool push_value);
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
