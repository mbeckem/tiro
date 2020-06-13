#include "compiler/semantics/type_table.hpp"

#include "compiler/ast/ast.hpp"

namespace tiro {

std::string_view to_string(ValueType type) {
    switch (type) {
#define TIRO_CASE(X)   \
    case ValueType::X: \
        return #X;

        TIRO_CASE(None)
        TIRO_CASE(Value)
        TIRO_CASE(Never)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid value type.");
}

TypeTable::TypeTable() = default;

TypeTable::~TypeTable() = default;

TypeTable::TypeTable(TypeTable&&) noexcept = default;

TypeTable& TypeTable::operator=(TypeTable&&) noexcept = default;

void TypeTable::register_type(AstId node, ValueType type) {
    TIRO_DEBUG_ASSERT(node, "Invalid node id.");
    TIRO_DEBUG_ASSERT(
        types_.find(node) == types_.end(), "The node was already registered with a type.");
    types_.emplace(node, type);
}

std::optional<ValueType> TypeTable::find_type(AstId node) const {
    if (auto pos = types_.find(node); pos != types_.end())
        return pos->second;
    return std::nullopt;
}

ValueType TypeTable::get_type(AstId node) const {
    auto type = find_type(node);
    TIRO_DEBUG_ASSERT(type, "Failed to find type for node.");
    return *type;
}

} // namespace tiro
