#ifndef TIRO_SEMANTICS_SCOPE_BUILDER_HPP
#define TIRO_SEMANTICS_SCOPE_BUILDER_HPP

#include "tiro/ast/ast.hpp"
#include "tiro/compiler/reset_value.hpp"
#include "tiro/semantics/fwd.hpp"

namespace tiro {

class StringTable;
class Diagnostics;

} // namespace tiro

namespace tiro {

/// Builds the symbol table and resolves all references (name -> declared symbol).
SymbolTable
resolve_symbols(AstNode* root, const StringTable& strings, Diagnostics& diag);

} // namespace tiro

#endif // TIRO_SEMANTICS_SCOPE_BUILDER_HPP
