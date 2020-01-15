#ifndef HAMMER_COMPILER_SYNTAX_AST_HPP
#define HAMMER_COMPILER_SYNTAX_AST_HPP

#include "hammer/compiler/fwd.hpp"
#include "hammer/compiler/source_reference.hpp"
#include "hammer/core/function_ref.hpp"
#include "hammer/core/ref_counted.hpp"
#include "hammer/core/type_traits.hpp"

#include <memory>
#include <vector>

namespace hammer::compiler {

template<typename T>
struct NodeTraits : undefined_type {};

std::string_view to_string(NodeType type);

/// Base class of all ast nodes.
///
/// Nodes form an inheritance hierarchy. The dynamic (i.e. most derived)
/// type of a node can always be determined by calling node->type().
/// Use the functions `try_cast` and `must_cast` to cast nodes between types -
/// `dynamic_cast<>` is not neccessary.
class Node : public RefCounted {
public:
    virtual ~Node();

    /// Returns the dynamic type of the node.
    constexpr NodeType type() const { return type_; }

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
    explicit Node(NodeType type);

private:
    // Type tag
    const NodeType type_;

    // True if the node has internal errors.
    bool has_error_ = false;

    // Position of the first node in the source code.
    SourceReference start_;
};

/// Returns a compact string representation of the given node.
std::string format_node(Node* node, const StringTable& strings);

/// Returns a string representation of the complete tree rooted at the given node.
std::string format_tree(Node* node, const StringTable& strings);

/// Use HAMMER_VISITOR_OVERRIDE when you inherit from DefaultNodeVisitor
/// to ensure that you actually override parent class methods.
///
/// Using virtual in the base class and override in the overriding
/// visitor prevents errors at compile time (misspellings).
/// The virtual keyword is only active in debug mode - the real implementation
/// is based on templates and static dispatch.
#ifdef HAMMER_DEBUG
#    define HAMMER_VISITOR_DECL virtual
#    define HAMMER_VISITOR_OVERRIDE override
#else
#    define HAMMER_VISITOR_DECL
#    define HAMMER_VISITOR_OVERRIDE
#endif

/// The default implementation for every node type simply invokes the
/// implementation for the node's base class.
///
/// Implemented in the generated header file.
template<typename Derived, typename... Arguments>
class DefaultNodeVisitor;

/// Base class for all node types that contain a list of child nodes.
template<typename T>
class NodeListBase {
public:
    using node_type = T;
    using value_type = NodePtr<T>;

    class NodeIterator {
    public:
        using value_type = NodePtr<T>;
        using reference = value_type;
        using iterator_category = std::forward_iterator_tag;

        NodeIterator() = default;

        T* operator*() const { return static_cast<T*>(pos_->get()); }

        NodeIterator& operator++() {
            ++pos_;
            return *this;
        }

        NodeIterator operator++(int) {
            NodeIterator temp(*this);
            ++pos_;
            return temp;
        }

        bool operator==(const NodeIterator& other) const {
            return pos_ == other.pos_;
        }

        bool operator!=(const NodeIterator& other) const {
            return pos_ != other.pos_;
        }

    private:
        friend NodeListBase;

        explicit NodeIterator(std::vector<NodePtr<Node>>::const_iterator pos)
            : pos_(std::move(pos)) {}

        std::vector<NodePtr<Node>>::const_iterator pos_;
    };

public:
    NodeIterator begin() const { return NodeIterator(entries_.begin()); }
    NodeIterator end() const { return NodeIterator(entries_.end()); }
    auto entries() const { return IterRange(begin(), end()); }

    size_t size() const { return entries_.size(); }

    T* get(size_t index) const {
        HAMMER_ASSERT(index < entries_.size(), "Index out of bounds.");
        return static_ref_cast<T>(entries_[index]);
    }

    void set(size_t index, T* entry) {
        HAMMER_ASSERT(index < entries_.size(), "Index out of bounds.");
        entries_[index] = ref(entry);
    }

    void append(T* entry) { entries_.push_back(ref(entry)); }

    void remove(size_t index) {
        HAMMER_ASSERT(index < entries_.size(), "Index out of bounds.");
        entries_.erase(entries_.begin() + index);
    }

private:
    // Use same vector type to save on generated code size.
    std::vector<NodePtr<Node>> entries_;
};

/// The operator used in a unary operation.
enum class UnaryOperator : int {
    // Arithmetic
    Plus,
    Minus,

    // Binary
    BitwiseNot,

    // Boolean
    LogicalNot
};

std::string_view to_string(UnaryOperator op);

/// The operator used in a binary operation.
enum class BinaryOperator : int {
    // Arithmetic
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulus,
    Power,

    // Binary
    LeftShift,
    RightShift,
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,

    // Boolean
    Less,
    LessEquals,
    Greater,
    GreaterEquals,
    Equals,
    NotEquals,
    LogicalAnd,
    LogicalOr,

    // Assigments
    Assign,
};

std::string_view to_string(BinaryOperator op);

/// Represents the kind of value produced by an expression.
/// Types are computed by the analyzer.
enum class ExprType : u8 {
    None,  ///< Never produces a value.
    Never, ///< Never returns normally; convertible to Value.
    Value, ///< Produces a value.
};

/// Returns true if the expression type is valid when a value is expected.
inline bool can_use_as_value(ExprType type) {
    return type == ExprType::Never || type == ExprType::Value;
}

std::string_view to_string(ExprType type);

/// Returns true if `from` points to an instance of node type `To`.
template<typename To, typename From>
HAMMER_FORCE_INLINE bool isa(From* from) {
    using from_t = remove_cvref_t<From>;
    using to_t = remove_cvref_t<To>;
    using traits = NodeTraits<to_t>;

    if (!from) {
        return false;
    }

    // No type check is required if we're casting to a base class.
    if constexpr (std::is_base_of_v<to_t, from_t>) {
        return true;
    }

    // Check the dynamic type against the node type (or node type range)
    // of the destination type.
    const NodeType type = from->type();
    if constexpr (traits::is_abstract) {
        return (
            type >= traits::first_node_type && type <= traits::last_node_type);
    } else {
        return type == traits::node_type;
    }
}

template<typename To, typename From>
HAMMER_FORCE_INLINE bool isa(const Ref<From>& from) {
    return isa<To>(from.get());
}

/// Attempts to cast the node pointer `from` to a pointer to `To` by
/// inspecting the dynamic type of the node at `from`.
///
/// Fails if `from` is either null or if it does not point to a node of type `To`.
/// Returns a valid pointer on success and a null pointer on failure.
template<typename To, typename From>
HAMMER_FORCE_INLINE To* try_cast(From* from) {
    return isa<To>(from) ? static_cast<To*>(from) : nullptr;
}

template<typename To, typename From>
HAMMER_FORCE_INLINE Ref<To> try_cast(const Ref<From>& from) {
    return ref(try_cast<To>(from.get()));
}

/// Attempts to cast `from` to a pointer to node type `To` (like `try_cast`), but
/// fails with an assertion error if `from` cannot be cast to the desired type.
template<typename To, typename From>
HAMMER_FORCE_INLINE To* must_cast(From* from) {
    auto result = try_cast<To>(from);
    HAMMER_ASSERT(result, "must_cast: cast failed.");
    return result;
}

template<typename To, typename From>
HAMMER_FORCE_INLINE Ref<To> must_cast(const Ref<From>& from) {
    return ref(must_cast<To>(from.get()));
}

/// `from` must be either an instance of `To` or null.
template<typename To, typename From>
HAMMER_FORCE_INLINE To* must_cast_nullable(From* from) {
    return from ? must_cast<To>(from) : nullptr;
}

template<typename To, typename From>
HAMMER_FORCE_INLINE Ref<To> must_cast_nullable(const Ref<From>& from) {
    return ref(must_cast_nullable<To>(from.get()));
}

/// Calls the visitor with the actual (most derived) node type instance.
/// Implemented in the generated file.
template<typename T, typename Visitor, typename... Arguments>
HAMMER_FORCE_INLINE decltype(auto)
visit(T* node, Visitor&& visitor, Arguments&&... args);

/// Invokes the callback with the provided node, downcasted to its most
/// derived type. Returns whatever the callback returns.
template<typename T, typename Callback>
HAMMER_FORCE_INLINE decltype(auto) downcast(T* node, Callback&& callback);

/// Calls the visitor function for every child node of the given parent node.
void traverse_children(Node* node, FunctionRef<void(Node*)> visitor);

/// Calls the transform function for every (mutable) child node of the given parent node.
/// The children will be replaced with the return value of the transformer.
void transform_children(Node* node, FunctionRef<NodePtr<>(Node*)> transformer);

} // namespace hammer::compiler

#include "hammer/compiler/syntax/ast.generated.hpp"

namespace hammer::compiler {

template<>
struct NodeTraits<Node> {
    static constexpr bool is_abstract = true;
    static constexpr NodeType first_node_type = NodeType::FirstNode;
    static constexpr NodeType last_node_type = NodeType::LastNode;

    template<typename T, typename Visitor>
    static void traverse_children(T*, Visitor&&) {}

    template<typename T, typename Transform>
    static void transform_children(T*, Transform&&) {}
};

template<typename T>
struct NodeListTraits {
    template<typename Visitor>
    static void traverse_items(NodeListBase<T>* node, Visitor&& visitor) {
        [[maybe_unused]] const size_t size = node->size();
        for (size_t i = 0; i < node->size(); ++i) {
            visitor(node->get(i));
            HAMMER_ASSERT(size == node->size(),
                "Visitor function must not alter the number of items.");
        }
    }

    template<typename Transform>
    static void transform_items(NodeListBase<T>* node, Transform&& transform) {
        [[maybe_unused]] const size_t size = node->size();
        for (size_t i = 0; i < size; ++i) {
            auto t = transform(node->get(i));
            HAMMER_ASSERT(size == node->size(),
                "Transform function must not alter the number of items.");
            node->set(i, must_cast_nullable<T>(t));
        }
    }
};

} // namespace hammer::compiler

#include "hammer/compiler/syntax/ast.generated.ipp"

#endif // HAMMER_COMPILER_SYNTAX_AST_HPP
