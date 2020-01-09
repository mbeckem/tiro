#ifndef HAMMER_COMPILER_FWD_HPP
#define HAMMER_COMPILER_FWD_HPP

#include "hammer/core/ref_counted.hpp"

#include <memory>

namespace hammer::compiler {

enum class NodeType : int;

class Analyzer;
class Compiler;
class Diagnostics;
class Node;
class Scope;
class SymbolEntry;
class SymbolTable;
class StringTable;

template<typename T>
struct NodeTraits;

template<typename T = Node>
using NodePtr = Ref<T>;

template<typename T = Node>
using WeakNodePtr = WeakRef<T>;

using ScopePtr = Ref<Scope>;
using WeakScopePtr = WeakRef<Scope>;

using SymbolEntryPtr = Ref<SymbolEntry>;
using WeakSymbolEntryPtr = WeakRef<SymbolEntry>;

// TODO fwd declare other types

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_FWD_HPP
