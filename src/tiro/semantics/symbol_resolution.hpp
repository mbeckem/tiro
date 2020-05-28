#ifndef TIRO_SEMANTICS_SYMBOL_RESOLUTION_HPP
#define TIRO_SEMANTICS_SYMBOL_RESOLUTION_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/semantics/symbol_table.hpp"

namespace tiro {

class StringTable;
class Diagnostics;

} // namespace tiro

namespace tiro {

/// Builds the symbol table and resolves all references (name -> declared symbol).
SymbolTable
resolve_symbols(AstNode* root, StringTable& strings, Diagnostics& diag);

} // namespace tiro

#endif // TIRO_SEMANTICS_SYMBOL_RESOLUTION_HPP
