#ifndef TIRO_SEMANTICS_SYMBOLS_HPP
#define TIRO_SEMANTICS_SYMBOLS_HPP

#include "tiro/ast/node.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/semantics/fwd.hpp"

namespace tiro {

/// Represents the type of a symbol.
enum class SymbolType : u8 {
    Import = 1,
    Type,
    Function,
    Variable,
    FirstSymbolType = Import,
    LastSymbolType = Variable,
};

std::string_view to_string(SymbolType type);

/// Represents a declared symbol in the symbol tree.
/// Symbols are declared by language elements such as variable declarations
/// or type declarations.
class Symbol final {
private:
    friend Scope;

    struct ConstructorToken {};

public:
    explicit Symbol(ConstructorToken, Scope* parent, SymbolType type,
        InternedString name, AstId ast_id);
    ~Symbol();

    Symbol(const Symbol&) = delete;
    Symbol& operator=(const Symbol&) = delete;

    /// Returns the type of the symbol. Symbol types server as an annotation
    /// about the kind of syntax element that declared the symbol. For details,
    /// inspect the ast node directly.
    SymbolType type() const { return type_; }

    /// Returns the name of this symbol. The name may be invalid for anonymous symbols.
    InternedString name() const { return name_; }

    /// Ast node that declares this symbol.
    AstId ast_id() const { return ast_id_; }

    /// Whether the symbol can be modified or not.
    bool is_const() const { return is_const_; }
    void is_const(bool is_const) { is_const_ = is_const; }

    /// A symbol is captured if it is referenced from a nested closure function.
    bool captured() const { return captured_; }
    void captured(bool is_captured) { captured_ = is_captured; }

    /// A symbol is inactive if its declaration in its enclosing scope
    /// has not been reached yet.
    bool active() const { return active_; }
    void active(bool is_active) { active_ = is_active; }

private:
    Scope* parent_ = nullptr;
    SymbolType type_;
    InternedString name_;
    AstId ast_id_;

    // TODO: Make these flags.
    bool is_const_ = false;
    bool captured_ = false;
    bool active_ = false;
};

/// Represents the type of a scope.
enum class ScopeType : u8 {
    /// The global scope contains pre-defined symbols. The user cannot
    /// add additional items to that scope.
    Global,

    /// Contains file-level symbols such as imports, functions or variables.
    File,

    /// Contains function parameters.
    Parameters,

    /// Contains the declared symbol within a for statement (i.e. for (DECLS; ...; ...) {}).
    ForStatement,

    /// Contains the declared symbols in a block expression (i.e. { DECLS... }).
    Block,

    // Keep these in sync with the enumerators above:
    FirstScopeType = Global,
    LastScopeType = Block
};

std::string_view to_string(ScopeType type);

/// Represents a scope in the symbol tree. A scope may be multiple
/// sub scopes and an arbitary number of declared symbols (possibly anonymous).
/// Variable lookup typically involves walking the current scope and its parents for a name match.
class Scope final {
private:
    struct ConstructorToken {};

public:
    /// Constructs the global (root) scope.
    static ScopePtr make_root();

    explicit Scope(
        ConstructorToken, Scope* parent, ScopeType type, AstId ast_id);
    ~Scope();

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

    /// Returns the parent scope if this scope.
    Scope* parent() const { return parent_; }

    /// Returns true if this is the root scope.
    bool is_root() const { return level_ == 0; }

    /// Returns the nesting level of this scope (the root scope is at level 0).
    u32 level() const { return level_; }

    /// Returns the type of this scope. This information is derived from
    /// the AST node that originally started this scope. For details, inspect
    /// the actual ast node.
    ScopeType type() const { return type_; }

    /// The id of the ast node that started this scope. Note that the global scope
    /// has no associated ast node.
    AstId ast_id() const { return ast_id_; }

    /// Returns the nesting level of this scope. The higher the level,
    /// the deeper the nesting level. The global scope has level 0.
    u32 level() const { return level_; }

    /// Returns true if this scope is a body of a loop.
    /// Loop bodies are handled as special cases in the code generation
    /// phase w.r.t. captured variables (they are allocated in closure environments and not on the stack).
    /// TODO: Do we still need this? Check with codegen module.
    bool loop_body() const { return loop_body_; }
    void loop_body(bool is_loop_body) { loop_body_ = is_loop_body; }

    /// Returns true if `other` and `this` are the same nodes or if `other` is a strict ancestor of `this`.
    bool is_ancestor(const Scope* other) const;

    /// Returns true if `other` can be reached by walking the parent pointers starting
    /// from this node's parent.
    bool is_strict_ancestor(const Scope* other) const;

    /// The child scopes of this scope.
    auto children() const {
        auto view = IterRange(children_.begin(), children_.end());
        auto transform = [&](const auto& scope) -> NotNull<Scope*> {
            return TIRO_NN(scope.get());
        };
        return TransformView(view, transform);
    }

    /// Returns the number of child scopes.
    u32 child_count() const { return children_.size(); }

    /// Constructs a new scope and adds it as a child to this scope. The child is owned by this scope.
    /// Arguments are passed to the child scope's constructor.
    NotNull<Scope*> add_child(ScopeType type, AstId ast_id);

    /// Returns a range over the symbol entires in this scope.
    auto entries() const {
        auto view = IterRange(entries_.begin(), entries_.end());
        auto transform = [&](const auto& entry) -> NotNull<Symbol*> {
            return TIRO_NN(entry.get());
        };
        return TransformView(view, transform);
    }

    /// Returns the number of symbol entries in this scope.
    u32 entry_count() const { return entries_.size(); }

    /// Constructs a new symbol and adds it as an entry to this scope. The entry is owned by this scope.
    /// Arguments are passed to the symbol's constructor. The name may be invalid if the new symbol
    /// represents an anonymous symbol.
    ///
    /// If the name is valid, insertion will fail (with return value nullptr) if an entry with that name already
    /// exists in this scope.
    Symbol* add_entry(SymbolType type, InternedString name, AstId ast_id);

    /// Attempts to find a symbol entry for the given name in this scope. Does not search in the parent scope.
    /// Returns nullptr if no symbol with that name exists in this scope.
    Symbol* find_local(InternedString name);

    /// Attempts to find a symbol entry for the given name in this scope or any of its parents.
    /// Returns (nullptr, nullptr) if no symbol with that name could be found. Otherwise, returns
    /// `(found_scope, found_symbol)`, where `found_scope` points to the containing scope and `found_symbol`
    /// points to the corresponding symbol entry in that scope.
    std::pair<Scope*, Symbol*> find(InternedString name);

private:
    Scope* parent_ = nullptr;
    ScopeType type_;
    AstId ast_id_;
    u32 level_ = 0;

    // TODO: Flags!
    bool loop_body_ = false;

    std::vector<ScopePtr> children_;
    std::vector<SymbolPtr> entries_;
    std::unordered_map<InternedString, u32, UseHasher> named_entries_;
};

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::SymbolType)
TIRO_ENABLE_FREE_TO_STRING(tiro::ScopeType);

#endif // TIRO_SEMANTICS_SYMBOLS_HPP
