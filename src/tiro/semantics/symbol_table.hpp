#ifndef TIRO_SEMANTICS_SYMBOL_TABLE_HPP
#define TIRO_SEMANTICS_SYMBOL_TABLE_HPP

#include "tiro/ast/node.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/core/id_type.hpp"
#include "tiro/core/index_map.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/semantics/fwd.hpp"

namespace tiro {

TIRO_DEFINE_ID(SymbolId, u32)
TIRO_DEFINE_ID(ScopeId, u32)

/// Represents the type of a symbol.
enum class SymbolType : u8 {
    Import = 1,
    Type,
    Function,
    Parameter,
    Variable,
    FirstSymbolType = Import,
    LastSymbolType = Variable,
};

std::string_view to_string(SymbolType type);

/// Represents the unique key for a declared symbol. Some AST nodes
/// may declare more than one symbol, so we have to disambiguate here.
class SymbolKey final {
public:
    static SymbolKey for_node(AstId node) { return {node, 0}; }

    static SymbolKey for_element(AstId node, u32 index) {
        return {node, index};
    }

    AstId node() const { return node_; }
    u32 index() const { return index_; }

    void build_hash(Hasher& h) const;
    void format(FormatStream& stream) const;

private:
    SymbolKey(AstId node, u32 index)
        : node_(node)
        , index_(index) {}

    AstId node_;
    u32 index_;
};

inline bool operator==(const SymbolKey& lhs, const SymbolKey& rhs) {
    return lhs.node() == rhs.node() && lhs.index() == rhs.index();
}

inline bool operator!=(const SymbolKey& lhs, const SymbolKey& rhs) {
    return !(lhs == rhs);
}

/// Represents a declared symbol in the symbol table.
/// Symbols are declared by language elements such as variable declarations
/// or type declarations.
class Symbol final {
public:
    explicit Symbol(ScopeId parent, SymbolType type, InternedString name,
        const SymbolKey& key)
        : parent_(parent)
        , type_(type)
        , name_(name)
        , key_(key) {}

    /// Returns the id of the parent scope.
    ScopeId parent() const { return parent_; }

    /// Returns the type of the symbol. Symbol types server as an annotation
    /// about the kind of syntax element that declared the symbol. For details,
    /// inspect the ast node directly.
    SymbolType type() const { return type_; }

    /// Returns the name of this symbol. The name may be invalid for anonymous symbols.
    InternedString name() const { return name_; }

    /// Ast node that declares this symbol.
    const SymbolKey& key() const { return key_; }

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
    ScopeId parent_;
    SymbolType type_;
    InternedString name_;
    SymbolKey key_;

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
    Function,

    /// Contains the declared symbol within a for statement (i.e. for (DECLS; ...; ...) {}).
    ForStatement,

    /// Contains block scoped variables.
    Block,

    // Keep these in sync with the enumerators above:
    FirstScopeType = Global,
    LastScopeType = Block
};

std::string_view to_string(ScopeType type);

/// Represents a scope in the symbol tree. A scope may have multiple
/// sub scopes and an arbitary number of declared symbols (possibly anonymous).
/// Variable lookup typically involves walking the current scope and its parents for a name match.
class Scope final {
public:
    explicit Scope(ScopeId parent, u32 level, SymbolId function, ScopeType type,
        AstId ast_id);

    Scope(Scope&&) noexcept = default;
    Scope& operator=(Scope&&) noexcept = default;

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

    /// Returns the parent scope of this scope.
    ScopeId parent() const { return parent_; }

    /// Returns true if this is the root scope.
    bool is_root() const { return level_ == 0; }

    /// Returns the nesting level of this scope (the root scope is at level 0).
    u32 level() const { return level_; }

    /// Returns the function this scope belongs to. Invalid if outside of a function.
    SymbolId function() const { return function_; }

    /// Returns the type of this scope. This information is derived from
    /// the AST node that originally started this scope. For details, inspect
    /// the actual ast node.
    ScopeType type() const { return type_; }

    /// The id of the ast node that started this scope. Note that the global scope
    /// has no associated ast node.
    AstId ast_id() const { return ast_id_; }

    /// The child scopes of this scope.
    auto children() const {
        return IterRange(children_.begin(), children_.end());
    }

    /// Returns the number of child scopes.
    u32 child_count() const { return children_.size(); }

    /// Returns a range over the symbol entires in this scope.
    auto entries() const { return IterRange(entries_.begin(), entries_.end()); }

    /// Returns the number of symbol entries in this scope.
    u32 entry_count() const { return entries_.size(); }

    /// Attempts to find a symbol entry for the given name in this scope. Does not search in the parent scope.
    /// Returns an invalid id if there is no such entry.
    SymbolId find_local(InternedString name) const;

private:
    friend SymbolTable;

    /// Adds the given child id to the list of children.
    void add_child(ScopeId child);

    /// Adds a new symbol entry to this scope. The name may be invalid. The scope maintains insertion
    /// order of its symbols.
    void add_entry(InternedString name, SymbolId sym);

private:
    ScopeId parent_;
    SymbolId function_;
    ScopeType type_;
    AstId ast_id_;
    u32 level_ = 0;

    std::vector<ScopeId> children_;
    std::vector<SymbolId> entries_;

    // TODO Better container
    std::unordered_map<InternedString, SymbolId, UseHasher> named_entries_;
};

class SymbolTable final {
public:
    SymbolTable();
    ~SymbolTable();

    SymbolTable(SymbolTable&&) noexcept;
    SymbolTable& operator=(SymbolTable&&) noexcept;

    /// Returns the id of the root scope.
    ScopeId root() const { return ScopeId(0); }

    /// Registers the given ast node as a reference to the given symbol.
    /// \pre The node must not already be referencing a symbol.
    void register_ref(AstId node, SymbolId sym);

    /// Returns the symbol previously associated with the given node (via `register_ref`),
    /// or an invalid id if there is no such symbol.
    SymbolId find_ref(AstId node) const;

    /// Like `find_ref`, but fails if no symbol was registered with the node.
    SymbolId get_ref(AstId node) const;

    /// Registers the given symbol with the symbol table.
    ///
    /// Returns an invalid id and does nothing if this symbol represents a named symbol (i.e. if it has a valid name)
    /// but the target scope already contains a symbol with that name.
    ///
    /// \pre The symbol's parent scope must be valid.
    /// \pre The symbol's key must be unique.
    SymbolId register_decl(const Symbol& sym);

    /// Returns the symbol associated with the given symbol key.
    /// Returns an invalid id if there is no such symbol.
    SymbolId find_decl(const SymbolKey& key) const;

    /// Like `find_decl`, but fails with an assertion error if no symbol was registered with the node.
    SymbolId get_decl(const SymbolKey& key) const;

    /// Creates a new scope and returns it's id.
    /// \pre The parent scope must be valid.
    /// \pre The scope's ast id must be unique.
    ScopeId register_scope(
        ScopeId parent, SymbolId function, ScopeType type, AstId node);

    /// Returns the scope id associated with the given node (via `register_scope`),
    /// or an invalid id if there is no such scope.
    ScopeId find_scope(AstId node) const;

    /// Like `find_scope`, but fails with an assertion error if no scope was registered with the node.
    ScopeId get_scope(AstId node) const;

    /// Attempts to find the given name in the specified scope. Does not inspect parent scopes.
    /// Returns the symbol's id on successor or an invalid id if the name was not found.
    SymbolId find_local_name(ScopeId scope, InternedString name) const;

    /// Attempts to find a symbol entry for the given name in the specified scope or any of its parents.
    /// Returns two invalid ids if no symbol with that name could be found. Otherwise, returns
    /// `(found_scope, found_symbol)`, where `found_scope` points to the containing scope and `found_symbol`
    /// points to the corresponding symbol entry in that scope.
    std::tuple<ScopeId, SymbolId>
    find_name(ScopeId scope, InternedString name) const;

    /// Returns true if `ancestor` is a actually a strict ancestor of `child`, i.e. if ancestor can be reached
    /// from child by following parent links, with `child != ancestor`.
    bool is_strict_ancestor(ScopeId ancestor, ScopeId child) const;

    ScopePtr operator[](ScopeId scope);
    SymbolPtr operator[](SymbolId sym);
    ConstScopePtr operator[](ScopeId scope) const;
    ConstSymbolPtr operator[](SymbolId sym) const;

private:
    // TODO: Better containers

    // Maps an ast node to the symbol referenced by that node.
    std::unordered_map<AstId, SymbolId, UseHasher> ref_index_;

    // Maps an ast node to the scope started by that node.
    std::unordered_map<AstId, ScopeId, UseHasher> scope_index_;

    // Maps symbol keys to defined symbols.
    std::unordered_map<SymbolKey, SymbolId, UseHasher> decl_index_;

    IndexMap<Symbol, IdMapper<SymbolId>> symbols_;
    IndexMap<Scope, IdMapper<ScopeId>> scopes_;
};

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::SymbolType)
TIRO_ENABLE_FREE_TO_STRING(tiro::ScopeType);

TIRO_ENABLE_MEMBER_FORMAT(tiro::SymbolKey);
TIRO_ENABLE_BUILD_HASH(tiro::SymbolKey);

#endif // TIRO_SEMANTICS_SYMBOL_TABLE_HPP
