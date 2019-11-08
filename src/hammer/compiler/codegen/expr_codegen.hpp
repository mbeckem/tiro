#ifndef HAMMER_COMPILER_CODEGEN_EXPR_CODEGEN_HPP
#define HAMMER_COMPILER_CODEGEN_EXPR_CODEGEN_HPP

#include "hammer/compiler/codegen/codegen.hpp"

namespace hammer {

/**
 * This class is responsible for compiling expressions to bytecode.
 */
class ExprCodegen final {
public:
    ExprCodegen(ast::Expr& expr, FunctionCodegen& func);

    void generate();

    ModuleCodegen& module() { return func_.module(); }

private:
    void gen_impl(ast::UnaryExpr& e);
    void gen_impl(ast::BinaryExpr& e);
    void gen_impl(ast::VarExpr& e);
    void gen_impl(ast::DotExpr& e);
    void gen_impl(ast::CallExpr& e);
    void gen_impl(ast::IndexExpr& e);
    void gen_impl(ast::IfExpr& e);
    void gen_impl(ast::ReturnExpr& e);
    void gen_impl(ast::ContinueExpr& e);
    void gen_impl(ast::BreakExpr& e);
    void gen_impl(ast::BlockExpr& e);
    void gen_impl(ast::NullLiteral& e);
    void gen_impl(ast::BooleanLiteral& e);
    void gen_impl(ast::IntegerLiteral& e);
    void gen_impl(ast::FloatLiteral& e);
    void gen_impl(ast::StringLiteral& e);
    void gen_impl(ast::SymbolLiteral& e);
    void gen_impl(ast::ArrayLiteral& e);
    void gen_impl(ast::TupleLiteral& e);
    void gen_impl(ast::MapLiteral& e);
    void gen_impl(ast::SetLiteral& e);
    void gen_impl(ast::FuncLiteral& e);

    void gen_assign(ast::BinaryExpr* assign);
    void gen_member_assign(ast::DotExpr* lhs, ast::Expr* rhs, bool push_value);
    void gen_index_assign(ast::IndexExpr* lhs, ast::Expr* rhs, bool push_value);

    void gen_logical_and(ast::Expr* lhs, ast::Expr* rhs);
    void gen_logical_or(ast::Expr* lhs, ast::Expr* rhs);

private:
    ast::Expr& expr_;
    FunctionCodegen& func_;
    CodeBuilder& builder_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace hammer

#endif // HAMMER_COMPILER_CODEGEN_EXPR_CODEGEN_HPP
