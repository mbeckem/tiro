#ifndef HAMMER_AST_NODE_HPP
#define HAMMER_AST_NODE_HPP

#include "hammer/ast/fwd.hpp"
#include "hammer/compiler/source_reference.hpp"
#include "hammer/core/casting.hpp"
#include "hammer/core/iter_range.hpp"
#include "hammer/core/type_traits.hpp"

#include <boost/intrusive/list.hpp>

#include <memory>

namespace hammer::ast {

/// When defined, maps the node kind value to the concrete node type.
template<NodeKind kind>
struct NodeKindToType : undefined_type {};

/// When defined, maps the node type to the nodes kind value.
template<typename NodeType>
struct NodeTypeToKind : undefined_type {};

/// When defined, maps the node type to the range of node kind values of its
/// derived (child) classes.  The child classes  must have contiguous
/// node kind values in [first_child, last_child].
template<typename NodeType>
struct NodeTypeToChildTypes : undefined_type {};

/**
 * Runtime type for ast nodes. Only concrete types have an associated value here.
 */
enum class NodeKind : i8 {
// Defines one enum value for every leaf type,
// and defines first_TYPE and last_TYPE for every abstract base type.
#define HAMMER_AST_LEAF_TYPE(Type, Parent) Type,
#define HAMMER_AST_TYPE_RANGE(Type, FirstType, LastType) \
    First##Type = (FirstType), Last##Type = (LastType),
#include "hammer/ast/node_types.inc"
};

/// Returns the name of the node kind.
std::string_view to_string(NodeKind kind);

// Support for linking nodes that are owned by the same parent.
using NodeBaseHook = boost::intrusive::list_base_hook<>;

/**
 * Base class of all ast nodes.
 */
class Node : public NodeBaseHook {
private:
    using NodeListType = boost::intrusive::list<Node, boost::intrusive::base_hook<NodeBaseHook>>;

public:
    using children_iterator = typename NodeListType::iterator;
    using const_children_iterator = typename NodeListType::const_iterator;

    using children_range = IterRange<children_iterator>;
    using const_children_range = IterRange<const_children_iterator>;

public:
    virtual ~Node();

    /// Returns the runtime type of the node.
    constexpr NodeKind kind() const { return kind_; }

    /// The parent of this node. The root does not have a parent.
    Node* parent() { return parent_; }
    const Node* parent() const { return parent_; }
    void parent(Node* parent) { parent_ = parent; }

    /// Traverse the linked list of child nodes.
    /// Functions return nullptr if there are no more nodes to visit.
    Node* first_child();
    Node* last_child();
    Node* next_child(Node* child);
    Node* prev_child(Node* child);

    /// Returns an iterable range over all children of this node.
    children_range children() { return {children_.begin(), children_.end()}; }
    const_children_range children() const { return {children_.begin(), children_.end()}; }

    size_t children_count() const { return children_.size(); }

    /// Returns true if this node represents an error.
    /// The node may not have the expected properties (for example, operands
    /// may be missing or invalid).
    bool has_error() const { return has_error_; }
    void has_error(bool err) { has_error_ = err; }

    // TODO
    SourceReference pos() const { return {}; }

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

protected:
    void dump_impl(NodeFormatter& fmt) const;

protected:
    explicit Node(NodeKind kind);

    // Adds a child to this node (after all existing children).
    // Returns a pointer to the child node.
    // This node takes ownership of its child.
    // Child may be null.
    template<typename Node>
    Node* add_child(std::unique_ptr<Node> child) {
        Node* result = child.get();
        add_child_impl(child.release());
        return result;
    }

    // Removes the child from this node. Child may be null.
    void remove_child(Node* child);

private:
    void add_child_impl(ast::Node* child);

private:
    // Type tag
    const NodeKind kind_;

    // Points back to the parent. The root and unlinked nodes have no parent.
    Node* parent_ = nullptr;

    // Children of a node are linked together in a doubly linked list.
    NodeListType children_;

    bool has_error_ = false;
};

// Registers the node type with the node kind system.
// The node kind (in enum class node_kind) must have the same name as the NodeType.
#define HAMMER_AST_LEAF_TYPE(NodeType, ParentType)            \
    template<>                                                \
    struct NodeKindToType<NodeKind::NodeType> {               \
        using type = NodeType;                                \
    };                                                        \
                                                              \
    template<>                                                \
    struct NodeTypeToKind<NodeType> {                         \
        static constexpr NodeKind value = NodeKind::NodeType; \
    };

// Registers the child (derived) classes of the NodeType.
#define HAMMER_AST_TYPE_RANGE(NodeType, FirstType, LastType)                \
    template<>                                                              \
    struct NodeTypeToChildTypes<NodeType> {                                 \
        static inline constexpr NodeKind first_child = NodeKind::FirstType; \
        static inline constexpr NodeKind last_child = NodeKind::LastType;   \
    };

#include "hammer/ast/node_types.inc"

void dump(const Node& n, std::ostream& os, const StringTable& strings, int indent = 0);

} // namespace hammer::ast

namespace hammer {

template<typename Target>
struct InstanceTestTraits<Target, std::enable_if_t<std::is_base_of_v<ast::Node, Target>>> {
    static constexpr bool is_instance(const ast::Node* n) {
        HAMMER_ASSERT(n, "Invalid node pointer.");

        // n is an instance of Target if the kind matches exactly.
        using to_kind = ast::NodeTypeToKind<Target>;
        if constexpr (!is_undefined<to_kind>) {
            if (to_kind::value == n->kind())
                return true;
        }

        // n is an instance of Target if it is one of the child classes.
        using to_child_kinds = ast::NodeTypeToChildTypes<Target>;
        if constexpr (!is_undefined<to_child_kinds>) {
            if (n->kind() >= to_child_kinds::first_child && n->kind() <= to_child_kinds::last_child)
                return true;
        }

        // Otherwise, it is not an instance.
        return false;
    }
};

} // namespace hammer

#endif // HAMMER_AST_NODE_HPP
