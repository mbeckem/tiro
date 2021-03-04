#ifndef TIRO_COMPILER_SYNTAX_SYNTAX_TREE_HPP
#define TIRO_COMPILER_SYNTAX_SYNTAX_TREE_HPP

#include "common/adt/index_map.hpp"
#include "common/adt/not_null.hpp"
#include "common/adt/span.hpp"
#include "common/format.hpp"
#include "common/id_type.hpp"
#include "compiler/ast_gen/fwd.hpp"
#include "compiler/syntax/fwd.hpp"
#include "compiler/syntax/token.hpp"

#include <absl/container/inlined_vector.h>

namespace tiro::next {

TIRO_DEFINE_ID(SyntaxNodeId, u32);

/* [[[cog
    from codegen.unions import define
    from codegen.syntax import SyntaxChild
    define(SyntaxChild.tag)
]]] */
enum class SyntaxChildType : u8 {
    Token,
    NodeId,
};

std::string_view to_string(SyntaxChildType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.syntax import SyntaxChild
    define(SyntaxChild)
]]] */
/// Represents the child of a syntax tree node.
class SyntaxChild final {
public:
    /// A token from the source code.
    using Token = tiro::next::Token;

    /// A node child.
    using NodeId = tiro::next::SyntaxNodeId;

    static SyntaxChild make_token(const Token& token);
    static SyntaxChild make_node_id(const NodeId& node_id);

    SyntaxChild(Token token);
    SyntaxChild(NodeId node_id);

    SyntaxChildType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Token& as_token() const;
    const NodeId& as_node_id() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto) visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    SyntaxChildType type_;
    union {
        Token token_;
        NodeId node_id_;
    };
};

bool operator==(const SyntaxChild& lhs, const SyntaxChild& rhs);
bool operator!=(const SyntaxChild& lhs, const SyntaxChild& rhs);
// [[[end]]]

/// Represents a node in the tree of syntax items.
/// Nodes typically have children, which are either concrete tokens or other syntax nodes.
///
/// Notes are mostly immutable. The only mutable data is the parent node id, which exists to make
/// traversing the tree easier. SyntaxNodes could be made fully immutable (and shared) in the future.
class SyntaxNode final {
public:
    using ChildStorage = absl::InlinedVector<SyntaxChild, 4>;

    // Indirection to save space, since most nodes do not contain errors
    using ErrorStorage = std::unique_ptr<std::vector<std::string>>;

    SyntaxNode(SyntaxType type, SourceRange range, ErrorStorage&& errors, ChildStorage&& children);
    ~SyntaxNode();

    SyntaxNode(SyntaxNode&&) noexcept;
    SyntaxNode& operator=(SyntaxNode&&) noexcept;

    /// Returns the syntax type of this node.
    SyntaxType type() const { return type_; }

    /// Returns the parent id of this node. The root node has no parent.
    SyntaxNodeId parent() const { return parent_; }

    /// Sets this node's parent. Typically only called during tree construction.
    void parent(SyntaxNodeId parent) { parent_ = parent; }

    /// The source range of this node is the (start, end) index interval within the source code
    /// that contains all its children.
    const SourceRange& range() const { return range_; }

    /// Returns the errors associated with this node.
    Span<const std::string> errors() const {
        const auto& storage = errors_;
        if (!storage)
            return {};
        return Span(*storage);
    }

    /// Returns the children of this node.
    Span<const SyntaxChild> children() const { return {children_.data(), children_.size()}; }

private:
    SyntaxType type_;
    SyntaxNodeId parent_;
    SourceRange range_;
    ErrorStorage errors_;
    ChildStorage children_;
};

/// The syntax tree contains the parsed syntax of a source text.
/// It points to the root syntax node and manages the lifetime of the entire tree.
class SyntaxTree final {
public:
    SyntaxTree(std::string_view source);
    ~SyntaxTree();

    SyntaxTree(SyntaxTree&&) noexcept;
    SyntaxTree& operator=(SyntaxTree&&) noexcept;

    /// Returns the full source code represented by this tree.
    std::string_view source() const;

    /// Returns the id of the root node.
    /// The root node, if it exists, always has type `Root`.
    SyntaxNodeId root_id() const;

    /// Sets the id of the root node.
    void root_id(SyntaxNodeId id);

    /// Constructs a new node and returns its id.
    SyntaxNodeId make(SyntaxNode&& node);

    NotNull<IndexMapPtr<SyntaxNode>> operator[](SyntaxNodeId id);
    NotNull<IndexMapPtr<const SyntaxNode>> operator[](SyntaxNodeId id) const;

private:
    std::string_view source_;
    SyntaxNodeId root_;
    IndexMap<SyntaxNode, IdMapper<SyntaxNodeId>> nodes_;
};

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.syntax import SyntaxChild
    implement_inlines(SyntaxChild)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto) SyntaxChild::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case SyntaxChildType::Token:
        return vis.visit_token(self.token_, std::forward<Args>(args)...);
    case SyntaxChildType::NodeId:
        return vis.visit_node_id(self.node_id_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid SyntaxChild type.");
}
// [[[end]]]

} // namespace tiro::next

TIRO_ENABLE_FREE_FORMAT(tiro::next::SyntaxChildType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::next::SyntaxChild)

#endif // TIRO_COMPILER_SYNTAX_SYNTAX_TREE_HPP
