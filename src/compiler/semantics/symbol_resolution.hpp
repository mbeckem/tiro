#ifndef TIRO_COMPILER_SEMANTICS_SYMBOL_RESOLUTION_HPP
#define TIRO_COMPILER_SEMANTICS_SYMBOL_RESOLUTION_HPP

#include "compiler/ast/fwd.hpp"
#include "compiler/semantics/fwd.hpp"

namespace tiro {

class Diagnostics;

} // namespace tiro

namespace tiro {

/// Builds the symbol table and resolves all references (name -> declared symbol).
void resolve_symbols(SemanticAst& ast, Diagnostics& diag);

} // namespace tiro

#endif // TIRO_COMPILER_SEMANTICS_SYMBOL_RESOLUTION_HPP
