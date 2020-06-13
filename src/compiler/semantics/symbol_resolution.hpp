#ifndef TIRO_COMPILER_SEMANTICS_SYMBOL_RESOLUTION_HPP
#define TIRO_COMPILER_SEMANTICS_SYMBOL_RESOLUTION_HPP

#include "compiler/ast/fwd.hpp"
#include "compiler/semantics/symbol_table.hpp"

namespace tiro {

class StringTable;
class Diagnostics;

} // namespace tiro

namespace tiro {

/// Builds the symbol table and resolves all references (name -> declared symbol).
SymbolTable resolve_symbols(AstNode* root, StringTable& strings, Diagnostics& diag);

} // namespace tiro

#endif // TIRO_COMPILER_SEMANTICS_SYMBOL_RESOLUTION_HPP
