#ifndef TIRO_COMPILER_SEMANTICS_SYMBOL_TABLE_HPP
#define TIRO_COMPILER_SEMANTICS_SYMBOL_TABLE_HPP

#include "common/adt/index_map.hpp"
#include "common/adt/not_null.hpp"
#include "common/defs.hpp"
#include "common/format.hpp"
#include "common/hash.hpp"
#include "common/id_type.hpp"
#include "compiler/ast/node.hpp"
#include "compiler/semantics/fwd.hpp"

#include "absl/container/flat_hash_map.h"

namespace tiro {

TIRO_DEFINE_ID(SymbolId, u32)
TIRO_DEFINE_ID(ScopeId, u32)

/* [[[cog
    from cog import outl
    from codegen.semantics import SymbolData
    from codegen.unions import define
    define(SymbolData.tag, SymbolData)
]]] */
/// Represents the type of a symbol.
enum class SymbolType : u8 {
    Import,
    TypeSymbol,
    Function,
    Parameter,
    Variable,
};

std::string_view to_string(SymbolType type);

/// Stores the data associated with a symbol.
class SymbolData final {
public:
    /// Represents an imported item.
    struct Import final {
        /// The imported item path.
        InternedString path;

        explicit Import(const InternedString& path_)
            : path(path_) {}
    };

    /// Represents a type.
    struct TypeSymbol final {};

    /// Represents a function item.
    struct Function final {};

    /// Represents a parameter value.
    struct Parameter final {};

    /// Represents a variable value.
    struct Variable final {};

    static SymbolData make_import(const InternedString& path);
    static SymbolData make_type_symbol();
    static SymbolData make_function();
    static SymbolData make_parameter();
    static SymbolData make_variable();

    SymbolData(Import import);
    SymbolData(TypeSymbol type_symbol);
    SymbolData(Function function);
    SymbolData(Parameter parameter);
    SymbolData(Variable variable);

    SymbolType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    void hash(Hasher& h) const;

    const Import& as_import() const;
    const TypeSymbol& as_type_symbol() const;
    const Function& as_function() const;
    const Parameter& as_parameter() const;
    const Variable& as_variable() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto) visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    SymbolType type_;
    union {
        Import import_;
        TypeSymbol type_symbol_;
        Function function_;
        Parameter parameter_;
        Variable variable_;
    };
};

bool operator==(const SymbolData& lhs, const SymbolData& rhs);
bool operator!=(const SymbolData& lhs, const SymbolData& rhs);
// [[[end]]]

/// Represents a declared symbol in the symbol table.
/// Symbols are declared by language elements such as variable declarations
/// or type declarations.
class Symbol final {
public:
    explicit Symbol(ScopeId parent, InternedString name, AstId node, const SymbolData& data)
        : parent_(parent)
        , name_(name)
        , data_(data)
        , node_(node) {}

    /// Returns the id of the parent scope.
    ScopeId parent() const { return parent_; }

    /// Returns the type of the symbol. Symbol types server as an annotation
    /// about the kind of syntax element that declared the symbol. For details,
    /// inspect the ast node directly.
    SymbolType type() const { return data_.type(); }

    /// Returns the name of this symbol. The name may be invalid for anonymous symbols.
    InternedString name() const { return name_; }

    /// Ast node that declares this symbol.
    AstId node() const { return node_; }

    /// Returns additional metadata associated with this symbol.
    const SymbolData& data() const { return data_; }

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

    bool exported() const { return exported_; }
    void exported(bool is_exported) { exported_ = is_exported; }

private:
    ScopeId parent_;
    InternedString name_;
    SymbolData data_;
    AstId node_;

    // TODO: Make these flags.
    bool is_const_ = false;
    bool captured_ = false;
    bool active_ = false;
    bool exported_ = false;
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
    explicit Scope(ScopeId parent, u32 level, SymbolId function, ScopeType type, AstId ast_id);

    Scope(Scope&&) noexcept = default;
    Scope& operator=(Scope&&) noexcept = default;

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

    /// Returns the parent scope of this scope.
    ScopeId parent() const { return parent_; }

    /// Returns true if this is the root scope.
    bool is_root() const { return level_ == 0; }

    /// Returns true if this scope belongs to the body of a loop.
    bool is_loop_scope() const { return is_loop_scope_; }
    void is_loop_scope(bool loop_scope) { is_loop_scope_ = loop_scope; }

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
    auto children() const { return IterRange(children_.begin(), children_.end()); }

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
    bool is_loop_scope_ = false;

    std::vector<ScopeId> children_;
    std::vector<SymbolId> entries_;

    absl::flat_hash_map<InternedString, SymbolId, UseHasher> named_entries_;
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

    /// Returns the symbol associated with the given node.
    /// Returns an invalid id if there is no such symbol.
    SymbolId find_decl(AstId node) const;

    /// Like `find_decl`, but fails with an assertion error if no symbol was registered with the node.
    SymbolId get_decl(AstId node) const;

    /// Creates a new scope and returns it's id.
    /// \pre The parent scope must be valid.
    /// \pre The scope's ast id must be unique.
    ScopeId register_scope(ScopeId parent, SymbolId function, ScopeType type, AstId node);

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
    std::tuple<ScopeId, SymbolId> find_name(ScopeId scope, InternedString name) const;

    /// Returns true if `ancestor` is a actually a strict ancestor of `child`, i.e. if ancestor can be reached
    /// from child by following parent links, with `child != ancestor`.
    bool is_strict_ancestor(ScopeId ancestor, ScopeId child) const;

    IndexMapPtr<Scope> operator[](ScopeId scope);
    IndexMapPtr<Symbol> operator[](SymbolId sym);
    IndexMapPtr<const Scope> operator[](ScopeId scope) const;
    IndexMapPtr<const Symbol> operator[](SymbolId sym) const;

private:
    // Maps an ast node to the symbol referenced by that node.
    absl::flat_hash_map<AstId, SymbolId, UseHasher> ref_index_;

    // Maps an ast node to the scope started by that node.
    absl::flat_hash_map<AstId, ScopeId, UseHasher> scope_index_;

    // Maps declaring nodes to defined symbols.
    absl::flat_hash_map<AstId, SymbolId, UseHasher> decl_index_;

    IndexMap<Symbol, IdMapper<SymbolId>> symbols_;
    IndexMap<Scope, IdMapper<ScopeId>> scopes_;
};

/* [[[cog
    from cog import outl
    from codegen.semantics import SymbolData
    from codegen.unions import implement_inlines
    implement_inlines(SymbolData)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto) SymbolData::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case SymbolType::Import:
        return vis.visit_import(self.import_, std::forward<Args>(args)...);
    case SymbolType::TypeSymbol:
        return vis.visit_type_symbol(self.type_symbol_, std::forward<Args>(args)...);
    case SymbolType::Function:
        return vis.visit_function(self.function_, std::forward<Args>(args)...);
    case SymbolType::Parameter:
        return vis.visit_parameter(self.parameter_, std::forward<Args>(args)...);
    case SymbolType::Variable:
        return vis.visit_variable(self.variable_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid SymbolData type.");
}
// [[[end]]]

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::SymbolType)
TIRO_ENABLE_FREE_TO_STRING(tiro::ScopeType);

#endif // TIRO_COMPILER_SEMANTICS_SYMBOL_TABLE_HPP
