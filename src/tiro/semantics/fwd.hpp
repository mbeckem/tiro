#ifndef TIRO_SEMANTICS_FWD_HPP
#define TIRO_SEMANTICS_FWD_HPP

#include "tiro/core/defs.hpp"

#include <memory>

// TODO core fwd decls
namespace tiro {
template<typename T>
class VecPtr;
}

namespace tiro {

class SymbolId;
class ScopeId;

class SymbolKey;

enum class SymbolType : u8;
class Symbol;

using SymbolPtr = VecPtr<Symbol>;
using ConstSymbolPtr = VecPtr<const Symbol>;

enum class ScopeType : u8;
class Scope;

using ScopePtr = VecPtr<Scope>;
using ConstScopePtr = VecPtr<const Scope>;

class SymbolTable;

enum class ValueType : u8;

class TypeTable;

} // namespace tiro

#endif // TIRO_SEMANTICS_FWD_HPP
