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

/* [[[cog
    from cog import outl
    from codegen.semantics import SymbolData
    from codegen.unions import implement
    implement(SymbolData.tag, SymbolData)
]]] */
std::string_view to_string(SymbolType type) {
    switch (type) {
    case SymbolType::Import:
        return "Import";
    case SymbolType::TypeSymbol:
        return "TypeSymbol";
    case SymbolType::Function:
        return "Function";
    case SymbolType::Parameter:
        return "Parameter";
    case SymbolType::Variable:
        return "Variable";
    }
    TIRO_UNREACHABLE("Invalid SymbolType.");
}

SymbolData SymbolData::make_import(const InternedString& path) {
    return {Import{path}};
}

SymbolData SymbolData::make_type_symbol() {
    return {TypeSymbol{}};
}

SymbolData SymbolData::make_function() {
    return {Function{}};
}

SymbolData SymbolData::make_parameter() {
    return {Parameter{}};
}

SymbolData SymbolData::make_variable() {
    return {Variable{}};
}

SymbolData::SymbolData(Import import)
    : type_(SymbolType::Import)
    , import_(std::move(import)) {}

SymbolData::SymbolData(TypeSymbol type_symbol)
    : type_(SymbolType::TypeSymbol)
    , type_symbol_(std::move(type_symbol)) {}

SymbolData::SymbolData(Function function)
    : type_(SymbolType::Function)
    , function_(std::move(function)) {}

SymbolData::SymbolData(Parameter parameter)
    : type_(SymbolType::Parameter)
    , parameter_(std::move(parameter)) {}

SymbolData::SymbolData(Variable variable)
    : type_(SymbolType::Variable)
    , variable_(std::move(variable)) {}

const SymbolData::Import& SymbolData::as_import() const {
    TIRO_DEBUG_ASSERT(
        type_ == SymbolType::Import, "Bad member access on SymbolData: not a Import.");
    return import_;
}

const SymbolData::TypeSymbol& SymbolData::as_type_symbol() const {
    TIRO_DEBUG_ASSERT(
        type_ == SymbolType::TypeSymbol, "Bad member access on SymbolData: not a TypeSymbol.");
    return type_symbol_;
}

const SymbolData::Function& SymbolData::as_function() const {
    TIRO_DEBUG_ASSERT(
        type_ == SymbolType::Function, "Bad member access on SymbolData: not a Function.");
    return function_;
}

const SymbolData::Parameter& SymbolData::as_parameter() const {
    TIRO_DEBUG_ASSERT(
        type_ == SymbolType::Parameter, "Bad member access on SymbolData: not a Parameter.");
    return parameter_;
}

const SymbolData::Variable& SymbolData::as_variable() const {
    TIRO_DEBUG_ASSERT(
        type_ == SymbolType::Variable, "Bad member access on SymbolData: not a Variable.");
    return variable_;
}

void SymbolData::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_import([[maybe_unused]] const Import& import) {
            stream.format("Import(path: {})", import.path);
        }

        void visit_type_symbol([[maybe_unused]] const TypeSymbol& type_symbol) {
            stream.format("TypeSymbol");
        }

        void visit_function([[maybe_unused]] const Function& function) {
            stream.format("Function");
        }

        void visit_parameter([[maybe_unused]] const Parameter& parameter) {
            stream.format("Parameter");
        }

        void visit_variable([[maybe_unused]] const Variable& variable) {
            stream.format("Variable");
        }
    };
    visit(FormatVisitor{stream});
}

void SymbolData::build_hash(Hasher& h) const {
    h.append(type());

    struct HashVisitor {
        Hasher& h;

        void visit_import([[maybe_unused]] const Import& import) { h.append(import.path); }

        void visit_type_symbol([[maybe_unused]] const TypeSymbol& type_symbol) { return; }

        void visit_function([[maybe_unused]] const Function& function) { return; }

        void visit_parameter([[maybe_unused]] const Parameter& parameter) { return; }

        void visit_variable([[maybe_unused]] const Variable& variable) { return; }
    };
    return visit(HashVisitor{h});
}

bool operator==(const SymbolData& lhs, const SymbolData& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const SymbolData& rhs;

        bool visit_import([[maybe_unused]] const SymbolData::Import& import) {
            [[maybe_unused]] const auto& other = rhs.as_import();
            return import.path == other.path;
        }

        bool visit_type_symbol([[maybe_unused]] const SymbolData::TypeSymbol& type_symbol) {
            [[maybe_unused]] const auto& other = rhs.as_type_symbol();
            return true;
        }

        bool visit_function([[maybe_unused]] const SymbolData::Function& function) {
            [[maybe_unused]] const auto& other = rhs.as_function();
            return true;
        }

        bool visit_parameter([[maybe_unused]] const SymbolData::Parameter& parameter) {
            [[maybe_unused]] const auto& other = rhs.as_parameter();
            return true;
        }

        bool visit_variable([[maybe_unused]] const SymbolData::Variable& variable) {
            [[maybe_unused]] const auto& other = rhs.as_variable();
            return true;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const SymbolData& lhs, const SymbolData& rhs) {
    return !(lhs == rhs);
}
// [[[end]]]

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

Scope::Scope(ScopeId parent, u32 level, SymbolId function, ScopeType type, AstId ast_id)
    : parent_(parent)
    , function_(function)
    , type_(type)
    , ast_id_(ast_id)
    , level_(level) {
    TIRO_DEBUG_ASSERT(type >= ScopeType::FirstScopeType && type <= ScopeType::LastScopeType,
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
    auto root_id = scopes_.push_back(Scope(ScopeId(), 0, SymbolId(), ScopeType::Global, AstId()));
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
    TIRO_DEBUG_ASSERT(decl_index_.count(sym.key()) == 0, "The symbol's key must be unique.");

    auto name = sym.name();
    auto parent_data = scopes_.ptr_to(sym.parent());
    if (name && parent_data->find_local(name))
        return SymbolId(); // Name exists.

    auto sym_id = symbols_.push_back(sym);
    parent_data->add_entry(name, sym_id);
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

ScopeId SymbolTable::register_scope(ScopeId parent, SymbolId function, ScopeType type, AstId node) {
    TIRO_DEBUG_ASSERT(
        parent && scopes_.in_bounds(parent), "The scope's parent scope must be valid.");
    TIRO_DEBUG_ASSERT(scope_index_.count(node) == 0, "The scope's ast node must be unique.");

    auto parent_data = scopes_.ptr_to(parent);
    u32 level = parent_data->level() + 1;

    auto child = scopes_.push_back(Scope(parent, level, function, type, node));
    parent_data->add_child(child);
    scope_index_.emplace(node, child);
    return child;
}

ScopeId SymbolTable::find_scope(AstId node) const {
    return get_id(scope_index_, node);
}

ScopeId SymbolTable::get_scope(AstId node) const {
    auto scope = find_scope(node);
    TIRO_DEBUG_ASSERT(scope, "Node was not associated with a scope.");
    return scope;
}

SymbolId SymbolTable::find_local_name(ScopeId scope, InternedString name) const {
    TIRO_DEBUG_ASSERT(scope && scopes_.in_bounds(scope), "Invalid scope id.");
    auto& scope_data = scopes_[scope];
    return scope_data.find_local(name);
}

std::tuple<ScopeId, SymbolId> SymbolTable::find_name(ScopeId scope, InternedString name) const {
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

bool SymbolTable::is_strict_ancestor(ScopeId ancestor, ScopeId child) const {
    while (1) {
        if (!child)
            return false;

        auto& child_data = scopes_[child];
        auto parent = child_data.parent();
        if (parent == ancestor)
            return true;

        child = parent;
    }
    return false;
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
