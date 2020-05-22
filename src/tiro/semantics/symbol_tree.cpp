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

Symbol::~Symbol() = default;

Symbol::Symbol(SymbolType type, InternedString name, AstId ast_id)
    : type_(type)
    , name_(name)
    , ast_id_(ast_id) {
    TIRO_DEBUG_ASSERT(type >= SymbolType::FirstSymbolType
                          && type <= SymbolType::LastSymbolType,
        "Invalid symbol type.");
}

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

} // namespace tiro
