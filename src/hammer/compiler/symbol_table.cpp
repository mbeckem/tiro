#include "hammer/compiler/symbol_table.hpp"

namespace hammer::compiler {

std::string_view to_string(ScopeType type) {
    switch (type) {
#define HAMMER_CASE(T) \
    case ScopeType::T: \
        return #T;

        HAMMER_CASE(Global)
        HAMMER_CASE(File)
        HAMMER_CASE(Parameters)
        HAMMER_CASE(ForStmtDecls)
        HAMMER_CASE(FunctionBody)
        HAMMER_CASE(LoopBody)
        HAMMER_CASE(Block)

#undef HAMMER_CASE
    }

    HAMMER_UNREACHABLE("Invalid scope type.");
}

SymbolEntry::SymbolEntry(
    ScopePtr scope, InternedString name, NodePtr<Decl> decl, PrivateTag)
    : scope_(scope)
    , name_(name)
    , decl_(std::move(decl)) {
    HAMMER_ASSERT_NOT_NULL(scope);
    HAMMER_ASSERT_NOT_NULL(decl_);
}

SymbolEntry::~SymbolEntry() {}

Scope::Scope(ScopeType type, SymbolTable* table, ScopePtr parent,
    NodePtr<FuncDecl> function, PrivateTag)
    : type_(type)
    , table_(table)
    , parent_(parent)
    , function_(std::move(function)) {
    if (parent) {
        depth_ = parent->depth() + 1;
    }
}

Scope::~Scope() {}

SymbolEntryPtr Scope::insert(const NodePtr<Decl>& decl) {
    HAMMER_ASSERT_NOT_NULL(decl);

    const InternedString name = decl->name();

    if (name && named_decls_.count(name))
        return nullptr;

    SymbolEntryPtr result = std::make_shared<SymbolEntry>(
        shared_from_this(), name, decl, SymbolEntry::PrivateTag());

    const u32 index = static_cast<u32>(decls_.size());
    decls_.push_back(result);
    named_decls_.emplace(name, index);
    return result;
}

SymbolEntryPtr Scope::find_local(InternedString name) {
    if (auto pos = named_decls_.find(name); pos != named_decls_.end()) {
        u32 index = pos->second;
        HAMMER_ASSERT(index < decls_.size(), "Decl index out of bounds.");
        return decls_[index];
    }
    return nullptr;
}

std::pair<SymbolEntryPtr, ScopePtr> Scope::find(InternedString name) {
    ScopePtr current = shared_from_this();
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
    ScopeType type, const ScopePtr& parent, const NodePtr<FuncDecl>& function) {
    HAMMER_ASSERT(!parent || parent->table() == this,
        "The parent scope must belong to the same table.");

    ScopePtr child = std::make_shared<Scope>(
        type, this, parent, function, Scope::PrivateTag());
    if (parent) {
        parent->children_.push_back(child);
    } else {
        roots_.push_back(child);
    }
    return child;
}

} // namespace hammer::compiler
