#ifndef TIRO_AST_NODE_HPP
#define TIRO_AST_NODE_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/ast/operators.hpp"
#include "tiro/compiler/source_reference.hpp"
#include "tiro/core/enum_flags.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/function_ref.hpp"
#include "tiro/core/id_type.hpp"
#include "tiro/core/iter_tools.hpp"
#include "tiro/core/string_table.hpp"

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
    TupleBinding = 1,
    VarBinding = 2,
    FirstBinding = 1,
    LastBinding = 2,
    FuncDecl = 3,
    ParamDecl = 4,
    VarDecl = 5,
    FirstDecl = 3,
    LastDecl = 5,
    BinaryExpr = 6,
    BlockExpr = 7,
    BreakExpr = 8,
    CallExpr = 9,
    ContinueExpr = 10,
    ElementExpr = 11,
    FuncExpr = 12,
    IfExpr = 13,
    ArrayLiteral = 14,
    BooleanLiteral = 15,
    FloatLiteral = 16,
    IntegerLiteral = 17,
    MapLiteral = 18,
    NullLiteral = 19,
    SetLiteral = 20,
    StringLiteral = 21,
    SymbolLiteral = 22,
    TupleLiteral = 23,
    FirstLiteral = 14,
    LastLiteral = 23,
    PropertyExpr = 24,
    ReturnExpr = 25,
    StringExpr = 26,
    StringGroupExpr = 27,
    UnaryExpr = 28,
    VarExpr = 29,
    FirstExpr = 6,
    LastExpr = 29,
    File = 30,
    NumericIdentifier = 31,
    StringIdentifier = 32,
    FirstIdentifier = 31,
    LastIdentifier = 32,
    EmptyItem = 33,
    FuncItem = 34,
    ImportItem = 35,
    VarItem = 36,
    FirstItem = 33,
    LastItem = 36,
    MapItem = 37,
    AssertStmt = 38,
    EmptyStmt = 39,
    ExprStmt = 40,
    ForStmt = 41,
    VarStmt = 42,
    WhileStmt = 43,
    FirstStmt = 38,
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

    AstId id() const { return id_; }
    void id(AstId new_id) { id_ = new_id; }

    SourceReference source() const { return source_; }
    void source(const SourceReference& new_source) { source_ = new_source; }

    AstNodeFlags flags() const { return flags_; }
    void flags(AstNodeFlags new_flags) { flags_ = new_flags; }

    bool has_error() const {
        return (flags_ & AstNodeFlags::HasError) != AstNodeFlags::None;
    }

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
    void mutate_children(MutableAstVisitor& visitor) {
        return do_mutate_children(visitor);
    }

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

    AstNodeList();
    ~AstNodeList();

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

    using UnderlyingIterator =
        typename std::vector<AstPtr<NodeType>>::const_iterator;
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

    bool operator==(const iterator& other) const {
        return iter_ == other._iter;
    }

    bool operator!=(const iterator& other) const {
        return iter_ != other.iter_;
    }

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

} // namespace tiro

TIRO_ENABLE_FREE_FORMAT(tiro::AstNodeFlags);
TIRO_ENABLE_FREE_TO_STRING(tiro::AstNodeType);
TIRO_ENABLE_FREE_TO_STRING(tiro::AccessType);

#endif // TIRO_AST_NODE_HPP
