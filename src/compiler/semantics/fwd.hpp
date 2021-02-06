#ifndef TIRO_COMPILER_SEMANTICS_FWD_HPP
#define TIRO_COMPILER_SEMANTICS_FWD_HPP

#include "common/defs.hpp"
#include "common/fwd.hpp"

#include <memory>

namespace tiro {

class SymbolId;
class ScopeId;

enum class SymbolType : u8;
class SymbolData;
class Symbol;

enum class ScopeType : u8;
class Scope;
class SymbolTable;

enum class ExprType : u8;

class TypeTable;

class SemanticAst;

} // namespace tiro

#endif // TIRO_COMPILER_SEMANTICS_FWD_HPP
