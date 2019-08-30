#ifndef HAMMER_AST_SCOPE_HPP
#define HAMMER_AST_SCOPE_HPP

#include "hammer/core/iter_range.hpp"
#include "hammer/compiler/string_table.hpp"

#include <unordered_map>

namespace hammer::ast {

class Node;
class Decl;

/// The type of a scope is derived from the ast element
/// that created the scope.
enum class ScopeKind {
    /// Top level scope
    GlobalScope,

    /// File level scope
    FileScope,

    /// Scope for the parameters of a function
    ParameterScope,

    /// Scope for variables introduced within a for loop
    ForStmtScope,

    /// Scope for block expressions (function bodies, loop/if bodies, etc..)
    BlockScope,
};

/**
 * Scopes contain the definitions of symbols.
 * They can be nested to implement lexical (or "static") scoping.
 */
class Scope {
public:
    class decl_iterator;

    using decl_range = IterRange<decl_iterator>;

public:
    explicit Scope(ScopeKind ScopeKind);
    ~Scope();

    /// Iterate over the declarations in this scope.
    decl_range declarations();

    /// Attempts to insert a new symbol with the given name in this scope.
    /// Returns true if the symbol was inserted, false if a symbol with that name was already defined
    /// in this scope.
    /// The scope does not take ownership of the symbol.
    bool insert(Decl* sym);

    /// Searches for a symbol with the given name in the current scope. Does not recurse into parent scopes.
    /// Returns a null pointer if no symbol was found.
    Decl* find_local(InternedString name);

    /// Queries this scope and its parents for a symbol with the given name.
    /// Returns the symbol and the scope in which the name was found. Returns two
    /// null pointers if the symbol was not found.
    std::pair<Decl*, Scope*> find(InternedString name);

    /// The parent scope, or null if this is the root scope.
    void parent_scope(Scope* sc);
    Scope* parent_scope() const { return parent_; }

    ScopeKind get_scope_kind() const { return kind_; }

    /// Returns the depth of this scope (the nesting level). The root scope has depth 0.
    int depth() const { return depth_; }

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

private:
    using private_symbol_iterator = std::unordered_map<InternedString, Decl*>::iterator;

private:
    const ScopeKind kind_;
    Scope* parent_ = nullptr;
    int depth_ = 0;

    // TODO should maintain insertion order for nicer variable layout.
    std::unordered_map<InternedString, Decl*> symbols_;
};

class Scope::decl_iterator {
public:
    using value_type = Decl;
    using reference = Decl&;
    using pointer = Decl*;
    using iterator_category = std::forward_iterator_tag;

public:
    decl_iterator() = default;

    reference operator*() const { return *pos_->second; }
    pointer operator->() const { return pos_->second; }

    decl_iterator& operator++() {
        pos_++;
        return *this;
    }

    decl_iterator operator++(int) {
        decl_iterator temp(*this);
        pos_++;
        return temp;
    }

    bool operator==(const decl_iterator& other) const { return pos_ == other.pos_; }
    bool operator!=(const decl_iterator& other) const { return pos_ != other.pos_; }

private:
    decl_iterator(private_symbol_iterator&& pos)
        : pos_(std::move(pos)) {}

    friend Scope;

private:
    private_symbol_iterator pos_;
};

inline Scope::decl_range Scope::declarations() {
    return {decl_iterator(symbols_.begin()), decl_iterator(symbols_.end())};
}

} // namespace hammer::ast

#endif // HAMMER_AST_SCOPE_HPP