#include "hammer/ast/scope.hpp"

#include "hammer/ast/decl.hpp"
#include "hammer/core/defs.hpp"

namespace hammer::ast {

Scope::Scope(ScopeKind kind)
    : kind_(kind) {}

Scope::~Scope() {}

bool Scope::insert(Decl* sym) {
    HAMMER_ASSERT_NOT_NULL(sym);

    HAMMER_ASSERT(
        sym->parent_scope() == nullptr, "Symbol already has a scope.");

    bool inserted = false;
    if (!sym->name()) {
        anon_symbols_.push_back(sym);
        inserted = true;
    } else {
        inserted = symbols_.try_emplace(sym->name(), sym).second;
    }

    if (inserted)
        sym->parent_scope(this);
    return inserted;
}

Decl* Scope::find_local(InternedString name) {
    if (auto pos = symbols_.find(name); pos != symbols_.end())
        return pos->second;
    return nullptr;
}

std::pair<Decl*, Scope*> Scope::find(InternedString name) {
    Scope* sc = this;
    do {
        if (Decl* sym = sc->find_local(name))
            return std::pair(sym, sc);

        sc = sc->parent_scope();
    } while (sc);
    return std::pair(nullptr, nullptr);
}

void Scope::parent_scope(Scope* sc) {
    parent_ = sc;
    depth_ = parent_ ? parent_->depth_ + 1 : 0;
}

} // namespace hammer::ast
