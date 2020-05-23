#include "tiro/semantics/symbol_tree.hpp"

namespace tiro {

std::string_view to_string(SymbolType type) {
    switch (type) {
#define TIRO_CASE(T)    \
    case SymbolType::T: \
        return #T;

        TIRO_CASE(Import)
        TIRO_CASE(Type)
        TIRO_CASE(Function)
        TIRO_CASE(Variable)

#undef TIRO_CASE
    }
    TIRO_UNREACHABLE("Invalid symbol type.");
}

Symbol::Symbol(ConstructorToken, Scope* parent, SymbolType type,
    InternedString name, AstId ast_id)
    : parent_(parent)
    , type_(type)
    , name_(name)
    , ast_id_(ast_id) {
    TIRO_DEBUG_ASSERT(type >= SymbolType::FirstSymbolType
                          && type <= SymbolType::LastSymbolType,
        "Invalid symbol type.");
}

Symbol::~Symbol() = default;

std::string_view to_string(ScopeType type) {
    switch (type) {
#define TIRO_CASE(T)   \
    case ScopeType::T: \
        return #T;

        TIRO_CASE(Global)
        TIRO_CASE(File)
        TIRO_CASE(Parameters)
        TIRO_CASE(ForStatement)
        TIRO_CASE(Block)

#undef TIRO_CASE
    }
    TIRO_UNREACHABLE("Invalid scope type.");
}

ScopePtr Scope::make_root() {
    return std::make_unique<Scope>(
        ConstructorToken(), nullptr, ScopeType::Global, AstId::invalid);
}

Scope::Scope(ConstructorToken, Scope* parent, ScopeType type, AstId ast_id)
    : parent_(parent)
    , type_(type)
    , ast_id_(ast_id)
    , level_(parent ? parent->level_ + 1 : 0) {
    TIRO_DEBUG_ASSERT(
        type >= ScopeType::FirstScopeType && type <= ScopeType::LastScopeType,
        "Invalid scope type.");
}

Scope::~Scope() {}

bool Scope::is_ancestor(const Scope* other) const {
    return this == other || is_strict_ancestor(other);
}

bool Scope::is_strict_ancestor(const Scope* other) const {
    const Scope* p = parent();
    while (1) {
        if (p == other)
            return true;

        if (!p)
            return false;

        p = p->parent();
    }
}

NotNull<Scope*> Scope::add_child(ScopeType type, AstId ast_id) {
    auto child = std::make_unique<Scope>(
        ConstructorToken(), this, type, ast_id);
    auto child_ptr = TIRO_NN(child.get());
    children_.push_back(std::move(child));
    return child_ptr;
}

Symbol* Scope::add_entry(SymbolType type, InternedString name, AstId ast_id) {
    if (name && named_entries_.count(name))
        return nullptr;

    auto entry = std::make_unique<Symbol>(
        Symbol::ConstructorToken(), this, type, name, ast_id);
    auto entry_ptr = TIRO_NN(entry.get());
    entries_.push_back(std::move(entry));

    if (name) {
        const u32 index = checked_cast<u32>(entries_.size() - 1);
        named_entries_.emplace(name, index);
    }

    return entry_ptr;
}

Symbol* Scope::find_local(InternedString name) {
    if (auto pos = named_entries_.find(name); pos != named_entries_.end()) {
        u32 index = pos->second;
        TIRO_DEBUG_ASSERT(
            index < entries_.size(), "Invalid index in entry map.");
        return entries_[index].get();
    }
    return nullptr;
}

std::pair<Scope*, Symbol*> Scope::find(InternedString name) {
    Scope* current = this;
    do {
        if (auto entry = current->find_local(name))
            return std::pair(this, entry);

        current = current->parent();
    } while (current);
    return std::pair(nullptr, nullptr);
}

} // namespace tiro
