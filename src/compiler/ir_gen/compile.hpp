#ifndef TIRO_COMPILER_IR_GEN_COMPILE_HPP
#define TIRO_COMPILER_IR_GEN_COMPILE_HPP

#include "compiler/ir_gen/func.hpp"
#include "compiler/ir_gen/fwd.hpp"

namespace tiro::ir {

/// Compiles the expression (which must represent a single left hand side value) and returns the target location.
/// This is being used to implement constructs such as "a = b" or "a.b = c".
TransformResult<AssignTarget> compile_target(NotNull<AstExpr*> expr, CurrentBlock& bb);

/// Compiles the given tuple literal expression as a set of assignment targets.
/// Used for tuple assignments such as "(a, b) = f()".
TransformResult<std::vector<AssignTarget>>
compile_tuple_targets(NotNull<AstTupleLiteral*> tuple, CurrentBlock& bb);

/// Compiles the target for the given simple variable declaration (i.e. "const foo = bar;").
AssignTarget compile_var_binding_target(NotNull<AstVarBindingSpec*> var, CurrentBlock& bb);

/// Compiles the target for the given tuple binding declaration (i.e. "const (foo, bar) = baz;").
std::vector<AssignTarget>
compile_tuple_binding_targets(NotNull<AstTupleBindingSpec*> tuple, CurrentBlock& bb);

/// Compiles tuple assignment, i.e. "(a, b, c) = tuple".
void compile_tuple_assign(const std::vector<AssignTarget>& targets, InstId tuple, CurrentBlock& bb);

/// Compiles the assignment "lhs = rhs" where lhs is the left hand side of a binding.
void compile_spec_assign(NotNull<AstBindingSpec*> spec, InstId rhs, CurrentBlock& bb);

/// Compiles the assignment expression "lhs = rhs" and returns the result.
InstResult compile_assign_expr(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb);

/// Compiles the compound assignment operator, e.g. "lhs += rhs";
InstResult compile_compound_assign_expr(
    BinaryOpType op, NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb);

/// Compiles the variable declaration and returns the result.
OkResult compile_var_decl(NotNull<AstVarDecl*> decl, CurrentBlock& bb);

/// Compiles the given statement and returns the result.
/// Returns false if the statement terminated control flow, i.e.
/// if the following code would be unreachable.
OkResult compile_stmt(NotNull<AstStmt*> stmt, CurrentBlock& bb);

/// Compiles the given expression. Might not return a value (e.g. unreachable).
/// May return an invalid local id if no value is required (MaybeInvalid flag set in options).
InstResult compile_expr(NotNull<AstExpr*> expr, ExprOptions options, CurrentBlock& bb);

/// Compiles the given value and returns an SSA instruction that represents that value.
/// Performs some ad-hoc optimizations, so the resulting instruction will not necessarily have exactly
/// the given value. Instructions can be reused, so the returned id may not be new.
InstId compile_value(const Value& value, CurrentBlock& bb);

} // namespace tiro::ir

#endif // TIRO_COMPILER_IR_GEN_COMPILE_HPP
