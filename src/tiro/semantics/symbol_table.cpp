#include "tiro/semantics/symbol_table.hpp"

namespace tiro {

template<typename Map, typename Key, typename Value>
static bool add_unique(Map& map, const Key& key, const Value& value) {
    auto result = map.try_emplace(key, value);
    return result.second;
}

template<typename Map, typename Key>
static typename Map::mapped_type get_id(const Map& map, Key& key) {
    auto pos = map.find(key);
    if (pos == map.end())
        return {};

    return pos->second;
}

std::string_view to_string(SymbolType type) {
    switch (type) {
#define TIRO_CASE(T)    \
    case SymbolType::T: \
        return #T;

        TIRO_CASE(Import)
        TIRO_CASE(Type)
        TIRO_CASE(Function)
        TIRO_CASE(Parameter)
        TIRO_CASE(Variable)

#undef TIRO_CASE
    }
    TIRO_UNREACHABLE("Invalid symbol type.");
}

void SymbolKey::build_hash(Hasher& h) const {
    h.append(node_, index_);
}

void SymbolKey::format(FormatStream& stream) const {
    stream.format("SymbolKey({}, {})", node_, index_);
}

std::string_view to_string(ScopeType type) {
    switch (type) {
#define TIRO_CASE(T)   \
    case ScopeType::T: \
        return #T;

        TIRO_CASE(Global)
        TIRO_CASE(File)
        TIRO_CASE(Function)
        TIRO_CASE(ForStatement)
        TIRO_CASE(Block)

#undef TIRO_CASE
    }
    TIRO_UNREACHABLE("Invalid scope type.");
}

Scope::Scope(ScopeId parent, u32 level, ScopeType type, AstId ast_id)
    : parent_(parent)
    , type_(type)
    , ast_id_(ast_id)
    , level_(level) {
    TIRO_DEBUG_ASSERT(
        type >= ScopeType::FirstScopeType && type <= ScopeType::LastScopeType,
        "Invalid scope type.");
}

void Scope::add_child(ScopeId child) {
    TIRO_DEBUG_ASSERT(child, "Invalid scope.");
    children_.push_back(child);
}

void Scope::add_entry(InternedString name, SymbolId sym) {
    TIRO_DEBUG_ASSERT(sym, "Invalid symbol.");
    entries_.push_back(sym);
    if (name)
        named_entries_.emplace(name, sym);
}

SymbolId Scope::find_local(InternedString name) const {
    if (auto pos = named_entries_.find(name); pos != named_entries_.end())
        return pos->second;
    return SymbolId();
}

SymbolTable::SymbolTable() {
    auto root_id = scopes_.push_back(
        Scope(ScopeId(), 0, ScopeType::Global, AstId()));
    TIRO_DEBUG_ASSERT(root_id.value() == 0, "Root scope id must be 0.");
}

SymbolTable::~SymbolTable() = default;

SymbolTable::SymbolTable(SymbolTable&&) noexcept = default;

SymbolTable& SymbolTable::operator=(SymbolTable&&) noexcept = default;

void SymbolTable::register_ref(AstId node, SymbolId sym) {
    [[maybe_unused]] bool inserted = add_unique(ref_index_, node, sym);
    TIRO_DEBUG_ASSERT(inserted, "Node is already registered as a reference.");
}

SymbolId SymbolTable::find_ref(AstId node) const {
    return get_id(ref_index_, node);
}

SymbolId SymbolTable::get_ref(AstId node) const {
    auto sym = find_ref(node);
    TIRO_DEBUG_ASSERT(sym, "Node was not registered as a reference.");
    return sym;
}

SymbolId SymbolTable::register_decl(const Symbol& sym) {
    TIRO_DEBUG_ASSERT(sym.parent() && scopes_.in_bounds(sym.parent()),
        "The symbol's parent scope must be valid.");
    TIRO_DEBUG_ASSERT(
        decl_index_.count(sym.key()) == 0, "The symbol's key must be unique.");

    auto name = sym.name();
    auto& parent = scopes_[sym.parent()];
    if (name && parent.find_local(name))
        return SymbolId(); // Name exists.

    auto sym_id = symbols_.push_back(sym);
    parent.add_entry(name, sym_id);
    decl_index_.emplace(sym.key(), sym_id);
    return sym_id;
}

SymbolId SymbolTable::find_decl(const SymbolKey& key) const {
    return get_id(decl_index_, key);
}

SymbolId SymbolTable::get_decl(const SymbolKey& key) const {
    auto sym = find_decl(key);
    TIRO_DEBUG_ASSERT(sym, "Node was not registered as a declaration.");
    return sym;
}

ScopeId
SymbolTable::register_scope(ScopeId parent, ScopeType type, AstId node) {
    TIRO_DEBUG_ASSERT(parent && scopes_.in_bounds(parent),
        "The scope's parent scope must be valid.");
    TIRO_DEBUG_ASSERT(
        scope_index_.count(node) == 0, "The scope's ast node must be unique.");

    auto& parent_data = scopes_[parent];
    u32 level = parent_data.level() + 1;

    auto child = scopes_.push_back(Scope(parent, level, type, node));
    parent_data.add_child(child);
    scope_index_.emplace(node, child);
    return child;
}

ScopeId SymbolTable::find_scope(AstId node) const {
    return get_id(scope_index_, node);
}

ScopeId SymbolTable::get_scope(AstId node) const {
    auto scope = get_scope(node);
    TIRO_DEBUG_ASSERT(scope, "Node was not associated with a scope.");
    return scope;
}

SymbolId
SymbolTable::find_local_name(ScopeId scope, InternedString name) const {
    TIRO_DEBUG_ASSERT(scope && scopes_.in_bounds(scope), "Invalid scope id.");
    auto& scope_data = scopes_[scope];
    return scope_data.find_local(name);
}

std::tuple<ScopeId, SymbolId>
SymbolTable::find_name(ScopeId scope, InternedString name) const {
    TIRO_DEBUG_ASSERT(scope && scopes_.in_bounds(scope), "Invalid scope id.");

    auto current = scope;
    do {
        auto& data = scopes_[current];
        if (auto entry = data.find_local(name))
            return std::pair(current, entry);

        current = data.parent();
    } while (current);
    return std::pair(ScopeId(), SymbolId());
}

ScopePtr SymbolTable::operator[](ScopeId scope) {
    return scopes_.ptr_to(scope);
}

SymbolPtr SymbolTable::operator[](SymbolId sym) {
    return symbols_.ptr_to(sym);
}

ConstScopePtr SymbolTable::operator[](ScopeId scope) const {
    return scopes_.ptr_to(scope);
}

ConstSymbolPtr SymbolTable::operator[](SymbolId sym) const {
    return symbols_.ptr_to(sym);
}

} // namespace tiro
