#ifndef TIRO_COMPILER_AST_GEN_BUILD_AST_HPP
#define TIRO_COMPILER_AST_GEN_BUILD_AST_HPP

#include "common/adt/span.hpp"
#include "common/fwd.hpp"
#include "compiler/ast/fwd.hpp"
#include "compiler/fwd.hpp"
#include "compiler/syntax/fwd.hpp"

namespace tiro::next {

// TODO: Get rid of string tables?

AstPtr<AstNode>
build_program_ast(const SyntaxTree& program_tree, StringTable& strings, Diagnostics& diag);

AstPtr<AstExpr>
build_expr_ast(const SyntaxTree& expr_tree, StringTable& strings, Diagnostics& diag);

} // namespace tiro::next

#endif // TIRO_COMPILER_AST_GEN_BUILD_AST_HPP
