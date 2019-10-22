#ifndef HAMMER_AST_NODE_VISIT_HPP
#define HAMMER_AST_NODE_VISIT_HPP

#include "hammer/ast/node.hpp"

#include "hammer/ast/decl.hpp"
#include "hammer/ast/expr.hpp"
#include "hammer/ast/file.hpp"
#include "hammer/ast/literal.hpp"
#include "hammer/ast/root.hpp"
#include "hammer/ast/stmt.hpp"

namespace hammer::ast {

template<typename Node, typename Visitor>
decltype(auto) visit_impl(Node* node, Visitor&& v);

/**
 * The visitor will be invoked with the `n` argument casted to its concrete type.
 * Visitor must define a call operator for every possible type (all derived classes for the given node type).
 */
template<typename Node, typename Visitor>
HAMMER_FORCE_INLINE decltype(auto) visit(const Node& n, Visitor&& v) {
    return visit_impl<const Node>(&n, std::forward<Visitor>(v));
}

/**
 * The visitor will be invoked with the `n` argument casted to its concrete type.
 * Visitor must define a call operator for every possible type (all derived classes for the given node type).
 */
template<typename Node, typename Visitor>
HAMMER_FORCE_INLINE decltype(auto) visit(Node& n, Visitor&& v) {
    return visit_impl<Node>(&n, std::forward<Visitor>(v));
}

template<typename Node>
HAMMER_FORCE_INLINE constexpr std::array<NodeKind, 2> visitation_range() {
    using child_types = NodeTypeToChildTypes<Node>;
    if constexpr (is_undefined<child_types>) {
        constexpr NodeKind kind = NodeTypeToKind<Node>::value;
        return {kind, kind};
    } else {
        return {child_types::first_child, child_types::last_child};
    }
}

template<typename Node, typename Visitor>
HAMMER_FORCE_INLINE decltype(auto) visit_impl(Node* node, Visitor&& v) {
    HAMMER_ASSERT_NOT_NULL(node);

    using node_type = remove_cvref_t<Node>;

    constexpr auto kind_range = visitation_range<node_type>();

    switch (node->kind()) {

// Casts down to the concrete type and calls the visitors operator().
// Expands to a no-op if the type is actually unreachable (i.e. not a type
// derived from node_type.
#define HAMMER_AST_LEAF_TYPE(Type, ParentType)                          \
case NodeKind::Type: {                                                  \
    if constexpr (NodeKind::Type >= kind_range[0]                       \
                  && NodeKind::Type <= kind_range[1]) {                 \
        return v(*must_cast<Type>(node));                               \
    } else {                                                            \
        /* Must not happen (corrupted type information) */              \
        HAMMER_ASSERT(false, "Invalid node type for this base class."); \
    }                                                                   \
}
#include "hammer/ast/node_types.inc"
    }

    HAMMER_UNREACHABLE("Missing node type in switch statement.");
}

} // namespace hammer::ast

#endif // HAMMER_AST_NODE_VISIT_HPP
