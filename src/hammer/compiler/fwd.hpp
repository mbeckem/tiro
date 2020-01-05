#ifndef HAMMER_COMPILER_FWD_HPP
#define HAMMER_COMPILER_FWD_HPP

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
using NodePtr = std::shared_ptr<T>;

template<typename T = Node>
using WeakNodePtr = std::weak_ptr<T>;

using ScopePtr = std::shared_ptr<Scope>;
using WeakScopePtr = std::weak_ptr<Scope>;

using SymbolEntryPtr = std::shared_ptr<SymbolEntry>;
using WeakSymbolEntryPtr = std::weak_ptr<SymbolEntry>;

// TODO fwd declare other types

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_FWD_HPP
