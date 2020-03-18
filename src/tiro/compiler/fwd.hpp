#ifndef TIRO_COMPILER_FWD_HPP
#define TIRO_COMPILER_FWD_HPP

#include "tiro/core/ref_counted.hpp"

#include <memory>

namespace tiro {

enum class NodeType : int;

class Analyzer;
class Compiler;
class Diagnostics;
class Node;
class Scope;
class Symbol;
class SymbolTable;

template<typename T>
struct NodeTraits;

template<typename T = Node>
using NodePtr = Ref<T>;

template<typename T = Node>
using WeakNodePtr = WeakRef<T>;

using ScopePtr = Ref<Scope>;
using WeakScopePtr = WeakRef<Scope>;

using SymbolPtr = Ref<Symbol>;
using WeakSymbolPtr = WeakRef<Symbol>;

// TODO fwd declare other types

} // namespace tiro

#endif // TIRO_COMPILER_FWD_HPP
