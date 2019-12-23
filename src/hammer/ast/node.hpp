#ifndef HAMMER_AST_NODE_HPP
#define HAMMER_AST_NODE_HPP

#include "hammer/ast/fwd.hpp"
#include "hammer/compiler/source_reference.hpp"
#include "hammer/core/casting.hpp"
#include "hammer/core/iter_range.hpp"
#include "hammer/core/type_traits.hpp"

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

/**
 * Base class of all ast nodes.
 */
class Node {
public:
    virtual ~Node();

    /// Returns the runtime type of the node.
    constexpr NodeKind kind() const { return kind_; }

    /// The parent of this node. The root does not have a parent.
    Node* parent() { return parent_; }
    const Node* parent() const { return parent_; }
    void parent(Node* parent) { parent_ = parent; }

    /// Returns true if this node represents an error.
    /// The node may not have the expected properties (for example, operands
    /// may be missing or invalid).
    bool has_error() const { return has_error_; }
    void has_error(bool err) { has_error_ = err; }

    /// The position of the token that starts this node in the source code.
    /// May not be valid.
    SourceReference start() const { return start_; }
    void start(const SourceReference& start) { start_ = start; }

    // TODO: Also track node end

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

protected:
    explicit Node(NodeKind kind);

    void dump_impl(NodeFormatter& fmt) const;

    template<typename Visitor>
    void visit_children(Visitor&&){};

private:
    // Type tag
    const NodeKind kind_;

    // Points back to the parent. The root and unlinked nodes have no parent.
    Node* parent_ = nullptr;

    // True if the node has internal errors.
    bool has_error_ = false;

    // Position of the first node in the source code.
    SourceReference start_;
};

using NodeVectorBase = std::vector<std::unique_ptr<Node>>;

/// This class is used to store lists of node children.
/// We reuse the same node vector type to safe on the amount of generated code.
template<typename T>
class NodeVector {
public:
    class node_iterator;

    node_iterator begin() const { return nodes_.begin(); }
    node_iterator end() const { return nodes_.end(); }
    auto entries() const { return IterRange(begin(), end()); }

    size_t size() const { return nodes_.size(); }

    void push_back(std::unique_ptr<T> node) {
        nodes_.push_back(std::move(node));
    }

    T* get(size_t index) const {
        HAMMER_ASSERT(index < size(), "Index out of bounds.");
        return static_cast<T*>(nodes_[index].get());
    }

    template<typename Func>
    void for_each(Func&& fn) {
        for (size_t i = 0; i < size(); ++i) {
            fn(static_cast<T*>(nodes_[i].get()));
        }
    }

private:
    NodeVectorBase nodes_;
};

template<typename T>
class NodeVector<T>::node_iterator {
public:
    using value_type = T*;
    using reference = T*;
    using iterator_category = std::forward_iterator_tag;

public:
    node_iterator();

    reference operator*() const { return static_cast<T*>(pos_->get()); }

    node_iterator& operator++() {
        pos_++;
        return *this;
    }

    node_iterator operator++(int) {
        node_iterator temp(*this);
        pos_++;
        return temp;
    }

    bool operator==(const node_iterator& other) const {
        return pos_ == other.pos_;
    }

    bool operator!=(const node_iterator& other) const {
        return pos_ != other.pos_;
    }

private:
    node_iterator(NodeVectorBase::const_iterator&& pos)
        : pos_(std::move(pos)) {}

    friend NodeVector<T>;

private:
    NodeVectorBase::const_iterator pos_;
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

void dump(const Node& n, std::ostream& os, const StringTable& strings,
    int indent = 0);

} // namespace hammer::ast

namespace hammer {

template<typename Target>
struct InstanceTestTraits<Target,
    std::enable_if_t<std::is_base_of_v<ast::Node, Target>>> {
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
            if (n->kind() >= to_child_kinds::first_child
                && n->kind() <= to_child_kinds::last_child)
                return true;
        }

        // Otherwise, it is not an instance.
        return false;
    }
};

} // namespace hammer

#endif // HAMMER_AST_NODE_HPP
