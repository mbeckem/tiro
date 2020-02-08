#include "tiro/semantics/symbol_table.hpp"

namespace tiro::compiler {

std::string_view to_string(ScopeType type) {
    switch (type) {
#define TIRO_CASE(T)   \
    case ScopeType::T: \
        return #T;

        TIRO_CASE(Global)
        TIRO_CASE(File)
        TIRO_CASE(Parameters)
        TIRO_CASE(ForStmtDecls)
        TIRO_CASE(FunctionBody)
        TIRO_CASE(LoopBody)
        TIRO_CASE(Block)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid scope type.");
}

std::string_view to_string(SymbolType type) {
    switch (type) {
#define TIRO_CASE(T)    \
    case SymbolType::T: \
        return #T;

        TIRO_CASE(Global)
        TIRO_CASE(Module)
        TIRO_CASE(Parameter)
        TIRO_CASE(Local)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid symbol type.");
}

Symbol::Symbol(SymbolType type, InternedString name, Decl* decl,
    const ScopePtr& scope, PrivateTag)
    : type_(type)
    , name_(name)
    , decl_(decl)
    , scope_(scope) {
    TIRO_ASSERT_NOT_NULL(scope);
    TIRO_ASSERT_NOT_NULL(decl_);
}

Symbol::~Symbol() {}

Scope::Scope(ScopeType type, SymbolTable* table, const ScopePtr& parent,
    FuncDecl* function, PrivateTag)
    : type_(type)
    , table_(table)
    , parent_(parent)
    , function_(ref(function)) {
    if (parent) {
        depth_ = parent->depth() + 1;
    }
}

Scope::~Scope() {}

SymbolPtr Scope::insert(SymbolType type, Decl* decl) {
    TIRO_ASSERT_NOT_NULL(decl);

    const InternedString name = decl->name();

    if (name && named_decls_.count(name))
        return nullptr;

    SymbolPtr result = make_ref<Symbol>(
        type, name, decl, Ref(this), Symbol::PrivateTag());

    const u32 index = static_cast<u32>(decls_.size());
    decls_.push_back(result);
    named_decls_.emplace(name, index);
    return result;
}

SymbolPtr Scope::find_local(InternedString name) {
    if (auto pos = named_decls_.find(name); pos != named_decls_.end()) {
        u32 index = pos->second;
        TIRO_ASSERT(index < decls_.size(), "Decl index out of bounds.");
        return decls_[index];
    }
    return nullptr;
}

std::pair<SymbolPtr, ScopePtr> Scope::find(InternedString name) {
    ScopePtr current = Ref(this);
    do {
        if (auto entry = current->find_local(name))
            return std::pair(std::move(entry), std::move(current));

        current = current->parent();
    } while (current);
    return std::pair(nullptr, nullptr);
}

bool Scope::is_child_of(const ScopePtr& other) {
    ScopePtr p = parent();
    while (1) {
        if (p == other)
            return true;

        if (!p)
            return false;

        p = p->parent();
    }
}

SymbolTable::SymbolTable() {}

SymbolTable::~SymbolTable() {}

ScopePtr SymbolTable::create_scope(
    ScopeType type, const ScopePtr& parent, FuncDecl* function) {
    TIRO_ASSERT(!parent || parent->table() == this,
        "The parent scope must belong to the same table.");

    ScopePtr child = make_ref<Scope>(
        type, this, parent, function, Scope::PrivateTag());
    if (parent) {
        parent->children_.push_back(child);
    } else {
        roots_.push_back(child);
    }
    return child;
}

} // namespace tiro::compiler
