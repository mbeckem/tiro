#ifndef TIRO_SEMANTICS_SYMBOLS_HPP
#define TIRO_SEMANTICS_SYMBOLS_HPP

#include "tiro/ast/node.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/format.hpp"
#include "tiro/semantics/fwd.hpp"

namespace tiro {

enum class SymbolType : u8 {
    Import = 1,
    Type,
    Function,
    Variable,
    FirstSymbolType = Import,
    LastSymbolType = Variable,
};

std::string_view to_string(SymbolType type);

class Symbol final {
public:
    explicit Symbol(SymbolType type, InternedString name, AstId ast_id);
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
    SymbolType type_;
    InternedString name_;
    AstId ast_id_;

    // TODO: Make these flags.
    bool is_const_ = false;
    bool captured_ = false;
    bool active_ = false;
};

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

class Scope final {
public:
    explicit Scope(ScopeType type, AstId ast_id);
    ~Scope();

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

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

    /// The child scopes of this scope.
    auto children() const {
        auto view = IterRange(children_.begin(), children_.end());
        auto transform = [&](const auto& scope) -> Scope* {
            return scope.get();
        };
        return TransformView(view, transform);
    }

    u32 child_count() const { return children_.size(); }

    auto entries() const {
        auto view = IterRange(entries_.begin(), entries_.end());
        auto transform = [&](const auto& entry) -> Symbol* {
            return entry.get();
        };
        return TransformView(view, transform);
    }

private:
    ScopeType type_;
    AstId ast_id_;
    Scope* parent_ = nullptr;
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
