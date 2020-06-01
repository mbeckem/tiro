#ifndef TIRO_SEMANTICS_TYPE_CHECK_HPP
#define TIRO_SEMANTICS_TYPE_CHECK_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/semantics/fwd.hpp"
#include "tiro/semantics/type_table.hpp"

namespace tiro {

class StringTable;
class Diagnostics;

/// Performs type checking on the given AST.
///
/// Type checking is a very primitive algorithm right now. Because the language does
/// not have static types, almost any value can be used at any place. However, complexity
/// arises from the fact that BlockExprs and IfExpr may or may not return a value, so
/// we introduce an artifical "none" type for expressions that cannot be used in a value context.
TypeTable check_types(AstNode* root, Diagnostics& diag);

} // namespace tiro

#endif // TIRO_SEMANTICS_TYPE_CHECK_HPP
