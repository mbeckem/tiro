#ifndef TIRO_COMPILER_AST_NODE_HPP
#define TIRO_COMPILER_AST_NODE_HPP

#include "common/adt/function_ref.hpp"
#include "common/adt/not_null.hpp"
#include "common/entities/entity_id.hpp"
#include "common/enum_flags.hpp"
#include "common/format.hpp"
#include "common/hash.hpp"
#include "common/ranges/iter_tools.hpp"
#include "common/text/string_table.hpp"
#include "compiler/ast/fwd.hpp"
#include "compiler/source_db.hpp"

#include "absl/container/flat_hash_map.h"

#include <string_view>
#include <vector>

namespace tiro {

TIRO_DEFINE_ENTITY_ID(AstId, u32);

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
    ErrorExpr = 14,
    FieldExpr = 15,
    FuncExpr = 16,
    IfExpr = 17,
    ArrayLiteral = 18,
    BooleanLiteral = 19,
    FloatLiteral = 20,
    IntegerLiteral = 21,
    MapLiteral = 22,
    NullLiteral = 23,
    RecordLiteral = 24,
    SetLiteral = 25,
    StringLiteral = 26,
    SymbolLiteral = 27,
    TupleLiteral = 28,
    FirstLiteral = 18,
    LastLiteral = 28,
    ReturnExpr = 29,
    StringExpr = 30,
    TupleFieldExpr = 31,
    UnaryExpr = 32,
    VarExpr = 33,
    FirstExpr = 8,
    LastExpr = 33,
    File = 34,
    Identifier = 35,
    MapItem = 36,
    ExportModifier = 37,
    FirstModifier = 37,
    LastModifier = 37,
    Module = 38,
    RecordItem = 39,
    AssertStmt = 40,
    DeclStmt = 41,
    DeferStmt = 42,
    EmptyStmt = 43,
    ErrorStmt = 44,
    ExprStmt = 45,
    ForEachStmt = 46,
    ForStmt = 47,
    WhileStmt = 48,
    FirstStmt = 40,
    LastStmt = 48,
    FirstNode = 1,
    LastNode = 48,
    // [[[end]]]
};

std::string_view to_string(AstNodeType type);

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

    /// The node's source range.
    AbsoluteSourceRange range() const { return range_; }
    void range(const AbsoluteSourceRange& new_range) { range_ = new_range; }

    /// True if this node has an error (syntactic or semantic).
    bool has_error() const { return flags_.test(HasError); }
    void has_error(bool value) { flags_.set(HasError, value); }

    /// Support for non-modifying child traversal. Callback will be invoked for every
    /// direct child of this node.
    void traverse_children(FunctionRef<void(AstNode*)> callback) const {
        TIRO_DEBUG_ASSERT(callback, "Invalid callback.");
        return do_traverse_children(callback);
    }

    /// Support for mutable child traversal. The visitor will be invoked for every
    /// child node slot. Existing children may be replaced by the visitor implementation.
    void mutate_children(MutableAstVisitor& visitor) { return do_mutate_children(visitor); }

protected:
    virtual void do_traverse_children(FunctionRef<void(AstNode*)> callback) const;
    virtual void do_mutate_children(MutableAstVisitor& visitor);

protected:
    explicit AstNode(AstNodeType type);

private:
    enum Props {
        HasError = 1 << 0,
    };

private:
    const AstNodeType type_;
    AstId id_;
    AbsoluteSourceRange range_;
    Flags<Props> flags_;
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

    AstPtr<NodeType> take(size_t index) {
        TIRO_DEBUG_ASSERT(index < size(), "AstNodeList: Index out of bounds.");
        return std::move(items_[index]);
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

TIRO_ENABLE_FREE_TO_STRING(tiro::AstNodeType);
TIRO_ENABLE_FREE_TO_STRING(tiro::AccessType);

#endif // TIRO_COMPILER_AST_NODE_HPP
