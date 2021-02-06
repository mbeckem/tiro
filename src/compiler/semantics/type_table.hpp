#ifndef TIRO_COMPILER_SEMANTICS_TYPE_TABLE_HPP
#define TIRO_COMPILER_SEMANTICS_TYPE_TABLE_HPP

#include "common/format.hpp"
#include "common/hash.hpp"
#include "compiler/ast/node.hpp"
#include "compiler/semantics/fwd.hpp"

#include "absl/container/flat_hash_map.h"

#include <optional>
#include <string_view>

namespace tiro {

/// Represents the type of an expression.
enum class ExprType : u8 {
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

std::string_view to_string(ExprType type);

/// Returns true if the given type can be used in places where values are expected (e.g.
/// function arguments, nested expressions).
inline bool can_use_as_value(ExprType type) {
    return type == ExprType::Value || type == ExprType::Never;
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
    void register_type(AstId node, ExprType type);

    /// Returns the type previously registered with the given node (via `register_type`) or an
    /// empty optional if there is no such type.
    std::optional<ExprType> find_type(AstId node) const;

    /// Like `find_type`, but fails with an assertion error if no type information could be found.
    ExprType get_type(AstId node) const;

private:
    absl::flat_hash_map<AstId, ExprType, UseHasher> types_;
};

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::ExprType)

#endif // TIRO_COMPILER_SEMANTICS_TYPE_TABLE_HPP
