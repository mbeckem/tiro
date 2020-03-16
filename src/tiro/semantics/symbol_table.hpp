#ifndef TIRO_SEMANTICS_SYMBOL_TABLE_HPP
#define TIRO_SEMANTICS_SYMBOL_TABLE_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/ref_counted.hpp"
#include "tiro/core/string_table.hpp"
#include "tiro/syntax/ast.hpp"

#include <memory>
#include <unordered_map>

namespace tiro::compiler {

class Scope;
class Symbol;
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

enum class SymbolType {
    /// An import symbol.
    Import,

    /// A function symbol.
    Function,

    /// Variable at module scope.
    ModuleVar,

    /// A function parameter. Can only occur at function scope.
    ParameterVar,

    /// Variable local to a function.
    LocalVar,
};

std::string_view to_string(SymbolType type);

class Symbol : public RefCounted {
    friend Scope;

    struct PrivateTag {}; // make_shared needs a public constructor

public:
    explicit Symbol(SymbolType type, InternedString name, Decl* decl,
        const ScopePtr& scope, PrivateTag);
    ~Symbol();

    SymbolType type() const { return type_; }
    InternedString name() const { return name_; }
    Decl* decl() const { return decl_; }

    ScopePtr scope() const { return scope_.lock(); }

    // True if the scope entry can be referenced by an expression.
    bool active() const { return active_; }
    void active(bool value) { active_ = value; }

    // True if the symbol is referenced from nested functions.
    bool captured() const { return captured_; }
    void captured(bool value) { captured_ = value; }

private:
    SymbolType type_;
    InternedString name_;
    NodePtr<Decl> decl_;
    WeakScopePtr scope_;
    bool active_ = false;
    bool captured_ = false;
};

class Scope final : public RefCounted {
    friend SymbolTable;

    struct PrivateTag {}; // make_shared needs a public constructor

public:
    explicit Scope(ScopeType type, SymbolTable* table, const ScopePtr& parent,
        FuncDecl* function, PrivateTag);
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

    /// Returns the number of declarations in this scope, in the order in which they have been defined.
    u32 size() const { return static_cast<u32>(decls_.size()); }

    /// Attempts to insert a new symbol with the given name in this scope.
    /// Returns the new scope entry pointer on success.
    SymbolPtr insert(SymbolType type, Decl* decl);

    /// Searches for a declaration with the given name in the current scope. Does not recurse into parent scopes.
    /// Returns a null pointer if no symbol was found.
    SymbolPtr find_local(InternedString name);

    /// Queries this scope and its parents for a declaration with the given name.
    /// Returns the declaration and the scope in which the name was found. Returns two
    /// null pointers if the symbol was not found.
    std::pair<SymbolPtr, ScopePtr> find(InternedString name);

    /// Returns true iff *this is a child scope (recursivly) of `other`.
    bool is_child_of(const ScopePtr& other);

    Scope(const Scope&) = delete;
    const Scope& operator=(const Scope&) = delete;

private:
    const ScopeType type_;
    SymbolTable* table_;
    WeakScopePtr parent_;
    NodePtr<FuncDecl> function_;
    u32 depth_ = 0;
    std::vector<ScopePtr> children_;

    // TODO need a better index if scopes have to also remove decls again.
    // We maintain the insertion order of declarations.
    std::vector<SymbolPtr> decls_;
    std::unordered_map<InternedString, u32, UseHasher> named_decls_;
};

class SymbolTable final {
public:
    SymbolTable();
    ~SymbolTable();

    /// Creates a new scope object of the given type with the given parent.
    /// The parent is optional.
    ScopePtr
    create_scope(ScopeType type, const ScopePtr& parent, FuncDecl* function);

    SymbolTable(const SymbolTable&) = delete;
    SymbolTable& operator=(const SymbolTable&) = delete;

private:
    // Keep root nodes alive.
    std::vector<ScopePtr> roots_;
};

} // namespace tiro::compiler

TIRO_ENABLE_FREE_TO_STRING(tiro::compiler::ScopeType)
TIRO_ENABLE_FREE_TO_STRING(tiro::compiler::SymbolType)

#endif // TIRO_SEMANTICS_SYMBOL_TABLE_HPP
