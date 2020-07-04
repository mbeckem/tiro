#ifndef TIRO_COMPILER_AST_NODE_HPP
#define TIRO_COMPILER_AST_NODE_HPP

#include "common/enum_flags.hpp"
#include "common/format.hpp"
#include "common/function_ref.hpp"
#include "common/hash.hpp"
#include "common/id_type.hpp"
#include "common/iter_tools.hpp"
#include "common/not_null.hpp"
#include "common/string_table.hpp"
#include "compiler/ast/fwd.hpp"
#include "compiler/source_reference.hpp"

#include "absl/container/flat_hash_map.h"

#include <string_view>
#include <vector>

namespace tiro {

TIRO_DEFINE_ID(AstId, u32);

enum class AstNodeType : u8 {
    /* [[[cog
        import cog
        from codegen.ast import type_tags
        for (name, value) in type_tags():
            cog.outl(f"{name} = {value},");
    ]]] */
    Binding = 1,
    TupleBindingSpec = 2,
    VarBindingSpec = 3,
    FirstBindingSpec = 2,
    LastBindingSpec = 3,
    FuncDecl = 4,
    ImportDecl = 5,
    ParamDecl = 6,
    VarDecl = 7,
    FirstDecl = 4,
    LastDecl = 7,
    BinaryExpr = 8,
    BlockExpr = 9,
    BreakExpr = 10,
    CallExpr = 11,
    ContinueExpr = 12,
    ElementExpr = 13,
    FuncExpr = 14,
    IfExpr = 15,
    ArrayLiteral = 16,
    BooleanLiteral = 17,
    FloatLiteral = 18,
    IntegerLiteral = 19,
    MapLiteral = 20,
    NullLiteral = 21,
    SetLiteral = 22,
    StringLiteral = 23,
    SymbolLiteral = 24,
    TupleLiteral = 25,
    FirstLiteral = 16,
    LastLiteral = 25,
    PropertyExpr = 26,
    ReturnExpr = 27,
    StringExpr = 28,
    StringGroupExpr = 29,
    UnaryExpr = 30,
    VarExpr = 31,
    FirstExpr = 8,
    LastExpr = 31,
    File = 32,
    NumericIdentifier = 33,
    StringIdentifier = 34,
    FirstIdentifier = 33,
    LastIdentifier = 34,
    MapItem = 35,
    ExportModifier = 36,
    FirstModifier = 36,
    LastModifier = 36,
    AssertStmt = 37,
    DeclStmt = 38,
    DeferStmt = 39,
    EmptyStmt = 40,
    ExprStmt = 41,
    ForStmt = 42,
    WhileStmt = 43,
    FirstStmt = 37,
    LastStmt = 43,
    FirstNode = 1,
    LastNode = 43,
    // [[[end]]]
};

std::string_view to_string(AstNodeType type);

enum class AstNodeFlags : u32 {
    None = 0,
    HasError = 1 << 0,
};

TIRO_DEFINE_ENUM_FLAGS(AstNodeFlags)

void format(AstNodeFlags flags, FormatStream& stream);

/// Base class of all AST nodes.
class AstNode {
public:
    virtual ~AstNode();

    AstNode(const AstNode&) = delete;
    AstNode& operator=(const AstNode&) = delete;

    AstNodeType type() const { return type_; }

    /// The node's id. Should be unique after analysis.
    AstId id() const { return id_; }
    void id(AstId new_id) { id_ = new_id; }

    /// The node' entire source range, from start to finish. Contains all syntactic children.
    SourceReference source() const { return source_; }
    void source(const SourceReference& new_source) { source_ = new_source; }

    /// Collection of node properties.
    AstNodeFlags flags() const { return flags_; }
    void flags(AstNodeFlags new_flags) { flags_ = new_flags; }

    /// True if this node has an error (syntactic or semantic).
    bool has_error() const { return (flags_ & AstNodeFlags::HasError) != AstNodeFlags::None; }

    void has_error(bool value) {
        if (value) {
            flags_ |= AstNodeFlags::HasError;
        } else {
            flags_ &= ~AstNodeFlags::HasError;
        }
    }

    /// Support for non-modifying child traversal. Callback will be invoked for every
    /// direct child of this node.
    void traverse_children(FunctionRef<void(AstNode*)> callback) {
        TIRO_DEBUG_ASSERT(callback, "Invalid callback.");
        return do_traverse_children(callback);
    }

    /// Support for mutable child traversal. The visitor will be invoked for every
    /// child node slot. Existing children may be replaced by the visitor implementation.
    void mutate_children(MutableAstVisitor& visitor) { return do_mutate_children(visitor); }

protected:
    virtual void do_traverse_children(FunctionRef<void(AstNode*)> callback);
    virtual void do_mutate_children(MutableAstVisitor& visitor);

protected:
    explicit AstNode(AstNodeType type);

private:
    const AstNodeType type_;
    AstId id_;
    SourceReference source_;
    AstNodeFlags flags_ = AstNodeFlags::None;
};

/// A list of AST nodes, backed by a `std::vector`.
template<typename NodeType>
class AstNodeList final {
public:
    class iterator;
    using const_iterator = iterator;

    AstNodeList() = default;
    ~AstNodeList() = default;

    AstNodeList(AstNodeList&& other) noexcept = default;
    AstNodeList& operator=(AstNodeList&& other) noexcept = default;

    inline iterator begin() const;
    inline iterator end() const;
    auto items() const { return IterRange(begin(), end()); }

    bool empty() const { return items_.empty(); }

    size_t size() const { return items_.size(); }

    NodeType* get(size_t index) const {
        TIRO_DEBUG_ASSERT(index < size(), "AstNodeList: Index out of bounds.");
        return items_[index].get();
    }

    void set(size_t index, AstPtr<NodeType> node) {
        TIRO_DEBUG_ASSERT(index < size(), "AstNodeList: Index out of bounds.");
        return items_[index] = std::move(node);
    }

    void append(AstPtr<NodeType> node) { items_.push_back(std::move(node)); }

private:
    std::vector<AstPtr<NodeType>> items_;
};

template<typename NodeType>
class AstNodeList<NodeType>::iterator {
    friend AstNodeList;

    using UnderlyingIterator = typename std::vector<AstPtr<NodeType>>::const_iterator;
    using UnderlyingTraits = std::iterator_traits<UnderlyingIterator>;

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = NodeType*;
    using pointer = NodeType*;
    using reference = NodeType*;
    using difference_type = std::ptrdiff_t;

    iterator() = default;

    iterator& operator++() {
        ++iter_;
        return *this;
    }

    iterator operator++(int) {
        auto old = *this;
        ++iter_;
        return old;
    }

    NodeType* operator*() const { return iter_->get(); }

    bool operator==(const iterator& other) const { return iter_ == other._iter; }

    bool operator!=(const iterator& other) const { return iter_ != other.iter_; }

private:
    explicit iterator(UnderlyingIterator&& iter)
        : iter_(std::move(iter)) {}

    UnderlyingIterator iter_;
};

template<typename NodeType>
typename AstNodeList<NodeType>::iterator AstNodeList<NodeType>::begin() const {
    return iterator(items_.begin());
}

template<typename NodeType>
typename AstNodeList<NodeType>::iterator AstNodeList<NodeType>::end() const {
    return iterator(items_.end());
}

template<typename NodeType, typename Callback>
void traverse_list(const AstNodeList<NodeType>& list, Callback&& callback) {
    for (NodeType* child : list) {
        callback(child);
    }
}

enum class AccessType : u8 {
    Normal,
    Optional, // Null propagation, e.g. `instance?.member`.
};

std::string_view to_string(AccessType access);

/// Maps node ids to node instances.
// TODO: This is currently pretty unsafe, because nodes may be destroyed at any time. They
// are not owned by the map, but by their parents. When removing a child from a node, always
// remove it from the map as well. I would like to have a value based AST in the future, where
// all nodes are owned by the map (the approach taken by e.g. the IR Function or the SymbolTable).
class AstNodeMap final {
public:
    AstNodeMap();
    ~AstNodeMap();

    AstNodeMap(AstNodeMap&&) noexcept;
    AstNodeMap& operator=(AstNodeMap&&) noexcept;

    /// Registers all nodes reachable from `root`. All node ids must be unique.
    void register_tree(AstNode* root);

    /// Registers the given node with the map. The node stay alive while it is being referenced by the map.
    /// \pre The node's id must be unique.
    void register_node(NotNull<AstNode*> node);

    /// Removes the node associated with the given id from the map.
    /// Returns true if an entry for that id existed.
    /// \pre `id` must be a valid id.
    bool remove_node(AstId id);

    /// Attempts to find the ast node with the given id. Returns the node or nullptr if no node could be found.
    /// \pre `id` must be a valid id.
    AstNode* find_node(AstId id) const;

    /// Like `find_node`, but fails with an assertion error if the node could not be found.
    NotNull<AstNode*> get_node(AstId id) const;

private:
    absl::flat_hash_map<AstId, NotNull<AstNode*>, UseHasher> nodes_;
};

} // namespace tiro

TIRO_ENABLE_FREE_FORMAT(tiro::AstNodeFlags);
TIRO_ENABLE_FREE_TO_STRING(tiro::AstNodeType);
TIRO_ENABLE_FREE_TO_STRING(tiro::AccessType);

#endif // TIRO_COMPILER_AST_NODE_HPP
