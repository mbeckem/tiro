#ifndef HAMMER_COMPILER_SEMANTICS_SYMBOL_TABLE_HPP
#define HAMMER_COMPILER_SEMANTICS_SYMBOL_TABLE_HPP

#include "hammer/compiler/string_table.hpp"
#include "hammer/compiler/syntax/ast.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/core/ref_counted.hpp"

#include <memory>
#include <unordered_map>

namespace hammer::compiler {

class Scope;
class SymbolEntry;
class SymbolTable;

enum class ScopeType {
    /// Top level scope
    Global,

    /// File level scope
    File,

    /// Scope for the parameters of a function
    Parameters,

    /// Scope for variables introduced within a for loop
    ForStmtDecls,

    /// Function body scope
    FunctionBody,

    /// Scope introduced by the body of a loop
    LoopBody,

    /// Scope for block expressions (function bodies, loop/if bodies, etc..)
    Block,
};

std::string_view to_string(ScopeType type);

class SymbolEntry : public RefCounted {
    friend Scope;

    struct PrivateTag {}; // make_shared needs a public constructor

public:
    explicit SymbolEntry(
        ScopePtr scope, InternedString name, NodePtr<Decl> decl, PrivateTag);
    ~SymbolEntry();

    ScopePtr scope() const { return scope_.lock(); }
    InternedString name() const { return name_; }
    const NodePtr<Decl>& decl() const { return decl_; }

    // True if the scope entry can be referenced by an expression.
    bool active() const { return active_; }
    void active(bool value) { active_ = value; }

    // True if the symbol is referenced from nested functions.
    bool captured() const { return captured_; }
    void captured(bool value) { captured_ = value; }

private:
    WeakScopePtr scope_;
    InternedString name_;
    NodePtr<Decl> decl_;
    bool active_ = false;
    bool captured_ = false;
};

class Scope final : public RefCounted {
    friend SymbolTable;

    struct PrivateTag {}; // make_shared needs a public constructor

public:
    explicit Scope(ScopeType type, SymbolTable* table, ScopePtr parent,
        NodePtr<FuncDecl> function, PrivateTag);
    ~Scope();

    constexpr ScopeType type() const { return type_; }

    /// Points back to the symbol table.
    SymbolTable* table() const { return table_; }

    /// Returns a pointer to the parent scope (if any).
    ScopePtr parent() const { return parent_.lock(); }

    /// Returns the depth of this scope (the nesting level). The root scope has depth 0.
    u32 depth() const { return depth_; }

    /// Returns a range over the child scopes of this scope.
    auto children() const {
        return IterRange(children_.begin(), children_.end());
    }

    /// Returns the function that contains this scope (may be null if the scope is
    /// outside a function).
    const NodePtr<FuncDecl>& function() const { return function_; }

    /// Returns a range over the local symbol entries.
    auto entries() const { return IterRange(decls_.begin(), decls_.end()); }

    /// Returns the number of declarations in this scope.
    u32 size() const { return static_cast<u32>(decls_.size()); }

    /// Attempts to insert a new symbol with the given name in this scope.
    /// Returns the new scope entry pointer on success.
    SymbolEntryPtr insert(const NodePtr<Decl>& decl);

    /// Searches for a declaration with the given name in the current scope. Does not recurse into parent scopes.
    /// Returns a null pointer if no symbol was found.
    SymbolEntryPtr find_local(InternedString name);

    /// Queries this scope and its parents for a declaration with the given name.
    /// Returns the declaration and the scope in which the name was found. Returns two
    /// null pointers if the symbol was not found.
    std::pair<SymbolEntryPtr, ScopePtr> find(InternedString name);

    /// Returns true iff *this is a child scope (recursivly) of `other`.
    bool is_child_of(const ScopePtr& other);

    Scope(const Scope&) = delete;
    const Scope& operator=(const Scope&) = delete;

private:
    // TODO insertion order.
    const ScopeType type_;
    SymbolTable* table_;
    WeakScopePtr parent_;
    NodePtr<FuncDecl> function_;
    u32 depth_ = 0;
    std::vector<ScopePtr> children_;

    // TODO need a better index if scopes have to also remove decls again.
    // We should maintain the insertion order of declarations.
    std::vector<SymbolEntryPtr> decls_;
    std::unordered_map<InternedString, u32, UseHasher> named_decls_;
};

class SymbolTable final {
public:
    SymbolTable();
    ~SymbolTable();

    /// Creates a new scope object of the given type with the given parent.
    /// The parent is optional.
    ScopePtr create_scope(ScopeType type, const ScopePtr& parent,
        const NodePtr<FuncDecl>& function);

    SymbolTable(const SymbolTable&) = delete;
    SymbolTable& operator=(const SymbolTable&) = delete;

private:
    // Keep root nodes alive.
    std::vector<ScopePtr> roots_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_SEMANTICS_SYMBOL_TABLE_HPP
