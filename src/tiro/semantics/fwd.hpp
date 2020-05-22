#ifndef TIRO_SEMANTICS_FWD_HPP
#define TIRO_SEMANTICS_FWD_HPP

#include "tiro/core/defs.hpp"

#include <memory>

namespace tiro {

enum class SymbolType : u8;
class Symbol;

using SymbolPtr = std::unique_ptr<Symbol>;

enum class ScopeType : u8;
class Scope;

using ScopePtr = std::unique_ptr<Scope>;

} // namespace tiro

#endif // TIRO_SEMANTICS_FWD_HPP
