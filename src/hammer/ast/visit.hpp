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

namespace detail {

template<typename Node, typename Visitor>
decltype(auto) visit_impl(Node* node, Visitor&& v);

template<typename Node, typename Visitor>
void visit_children_impl(Node* node, Visitor&& v);

} // namespace detail

/**
 * The visitor will be invoked with the `n` argument casted to its concrete type.
 * Visitor must define a call operator for every possible type (all derived classes for the given node type).
 */
template<typename Node, typename Visitor>
HAMMER_FORCE_INLINE decltype(auto) visit(Node&& n, Visitor&& v) {
    using type = std::remove_reference_t<Node>;
    return detail::visit_impl<type>(&n, std::forward<Visitor>(v));
}

/**
 * The visitor will be invoked once for every child node of `n`.
 * Visitor must defined a call operator that accepts node pointers.
 * 
 * TODO: Pointers vs references :(
 */
template<typename Node, typename Visitor>
HAMMER_FORCE_INLINE void visit_children(Node&& n, Visitor&& v) {
    using type = std::remove_reference_t<Node>;
    return detail::visit_children_impl<type>(&n, std::forward<Visitor>(v));
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

namespace detail {

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

// Used within visit_children_impl. First, we use visit_impl to
// downcast the node to its concrete type, then put the visitor
// into node.visit_children().
template<typename Visitor>
struct ChildVisitor {
    Visitor& v_;

    explicit ChildVisitor(Visitor& v)
        : v_(v) {}

    template<typename Node>
    HAMMER_FORCE_INLINE void operator()(Node&& n) const {
        n.visit_children(v_);
    }
};

template<typename Node, typename Visitor>
HAMMER_FORCE_INLINE void visit_children_impl(Node* node, Visitor&& v) {
    HAMMER_ASSERT_NOT_NULL(node);
    visit_impl<Node>(node, ChildVisitor(v));
}

} // namespace detail

} // namespace hammer::ast

#endif // HAMMER_AST_NODE_VISIT_HPP
