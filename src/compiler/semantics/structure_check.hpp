#ifndef TIRO_COMPILER_SEMANTICS_STRUCTURE_CHECK_HPP
#define TIRO_COMPILER_SEMANTICS_STRUCTURE_CHECK_HPP

#include "compiler/ast/fwd.hpp"
#include "compiler/semantics/fwd.hpp"

namespace tiro {

class Diagnostics;
class StringTable;

/// Checks the given ast node (and its descendants) for structural correctness.
void check_structure(
    AstNode* node, const SymbolTable& symbols, const StringTable& strings, Diagnostics& diag);

} // namespace tiro

#endif // TIRO_COMPILER_SEMANTICS_STRUCTURE_CHECK_HPP