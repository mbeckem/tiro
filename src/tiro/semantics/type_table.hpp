#ifndef TIRO_SEMANTICS_TYPE_TABLE_HPP
#define TIRO_SEMANTICS_TYPE_TABLE_HPP

#include "tiro/ast/node.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/semantics/fwd.hpp"

#include <optional>
#include <string_view>
#include <unordered_map>

namespace tiro {

/// Represents the type of an expression.
enum class ValueType : u8 {
    /// Does not produce a value. This is used for expressions that cannot
    /// return a value, such as an `if` expression with a missing `else` branch
    /// or a block expression whose last statement does not produce a value.
    None,

    /// Most expressions simply produce a single value.
    Value,

    /// An expression that never returns, such as `return x` or `break`.
    /// Expressions of this type can be used in places where a value is expected, since
    /// those places will never be reached.
    Never,
};

std::string_view to_string(ValueType type);

/// Returns true if the given type can be used in places where values are expected (e.g.
/// function arguments, nested expressions).
inline bool can_use_as_value(ValueType type) {
    return type == ValueType::Value || type == ValueType::Never;
}

/// Maps ast nodes to type information.
class TypeTable final {
public:
    TypeTable();
    ~TypeTable();

    TypeTable(TypeTable&&) noexcept;
    TypeTable& operator=(TypeTable&&) noexcept;

    /// Registers the given ast node with the specified value type.
    /// \pre The node id must be valid.
    /// \pre The node was not already registered.
    void register_type(AstId node, ValueType type);

    /// Returns the type previously registered with the given node (via `register_type`) or an
    /// empty optional if there is no such type.
    std::optional<ValueType> find_type(AstId node) const;

    /// Like `find_type`, but fails with an assertion error if no type information could be found.
    ValueType get_type(AstId node) const;

private:
    std::unordered_map<AstId, ValueType, UseHasher> types_;
};

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::ValueType)

#endif // TIRO_SEMANTICS_TYPE_TABLE_HPP
