#ifndef TIRO_IR_GEN_ASSIGN_HPP
#define TIRO_IR_GEN_ASSIGN_HPP

#include "tiro/ir_gen/func.hpp"
#include "tiro/ir_gen/fwd.hpp"

namespace tiro {

/// Compiles the expression (which must represent a single left hand side value) and returns the target location.
/// This is being used to implement constructs such as "a = b" or "a.b = c".
TransformResult<AssignTarget> compile_target(NotNull<AstExpr*> expr, CurrentBlock& bb);

/// Compiles the given tuple literal expression as a set of assignment targets.
/// Used for tuple assignments such as "(a, b) = f()".
TransformResult<std::vector<AssignTarget>>
compile_tuple_targets(NotNull<AstTupleLiteral*> tuple, CurrentBlock& bb);

/// Compiles the target for the given simple variable declaration (i.e. "const foo = bar;").
AssignTarget compile_var_binding_target(NotNull<AstVarBinding*> var, CurrentBlock& bb);

/// Compiles the target for the given tuple binding declaration (i.e. "const (foo, bar) = baz;").
std::vector<AssignTarget>
compile_tuple_binding_targets(NotNull<AstTupleBinding*> tuple, CurrentBlock& bb);

/// Compiles the assignment expression "lhs = rhs" and returns the result.
LocalResult compile_assign_expr(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb);

/// Compiles the compound assignment operator, e.g. "lhs += rhs";
LocalResult compile_compound_assign_expr(
    BinaryOpType op, NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb);

/// Compiles the variable declaration and returns the result.
OkResult compile_var_decl(NotNull<AstVarDecl*> decl, CurrentBlock& bb);

} // namespace tiro

#endif // TIRO_IR_GEN_ASSIGN_HPP
