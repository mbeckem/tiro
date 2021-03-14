#ifndef TIRO_COMPILER_AST_GEN_BUILD_AST_HPP
#define TIRO_COMPILER_AST_GEN_BUILD_AST_HPP

#include "common/adt/span.hpp"
#include "common/fwd.hpp"
#include "compiler/ast/fwd.hpp"
#include "compiler/fwd.hpp"
#include "compiler/syntax/fwd.hpp"

namespace tiro {

// FIXME: ast nodes need source ranges!

AstPtr<AstFile>
build_file_ast(const SyntaxTree& file_tree, StringTable& strings, Diagnostics& diag);

AstPtr<AstStmt>
build_item_ast(const SyntaxTree& item_tree, StringTable& strings, Diagnostics& diag);

AstPtr<AstStmt>
build_stmt_ast(const SyntaxTree& stmt_tree, StringTable& strings, Diagnostics& diag);

AstPtr<AstExpr>
build_expr_ast(const SyntaxTree& expr_tree, StringTable& strings, Diagnostics& diag);

} // namespace tiro

#endif // TIRO_COMPILER_AST_GEN_BUILD_AST_HPP
