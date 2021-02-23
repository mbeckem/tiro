#ifndef TIRO_COMPILER_AST_GEN_SYNTAX_TREE_HPP
#define TIRO_COMPILER_AST_GEN_SYNTAX_TREE_HPP

#include "common/adt/index_map.hpp"
#include "common/adt/not_null.hpp"
#include "common/adt/span.hpp"
#include "common/format.hpp"
#include "common/id_type.hpp"
#include "compiler/ast_gen/fwd.hpp"
#include "compiler/syntax/token.hpp"

#include <absl/container/inlined_vector.h>

namespace tiro::next {

TIRO_DEFINE_ID(SyntaxNodeId, u32);

/* [[[cog
    from codegen.unions import define
    from codegen.ast_gen import SyntaxChild
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
    from codegen.ast_gen import SyntaxChild
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
/// TODO: Where and when to emit errors?
class SyntaxNode final {
public:
    SyntaxNode(Span<const SyntaxChild> children);
    ~SyntaxNode();

    SyntaxNode(SyntaxNode&&) noexcept;
    SyntaxNode& operator=(SyntaxNode&&) noexcept;

    Span<const SyntaxChild> children() const { return {children_.data(), children_.size()}; }

private:
    absl::InlinedVector<SyntaxChild, 4> children_;
};

/// The syntax tree contains the parsed syntax of a source text.
/// It points to the root syntax node and manages the lifetime of the entire tree.
class SyntaxTree final {
public:
    SyntaxTree();
    ~SyntaxTree();

    SyntaxTree(SyntaxTree&&) noexcept;
    SyntaxTree& operator=(SyntaxTree&&) noexcept;

    /// Returns the id of the root node.
    SyntaxNodeId root_id() const;

    /// Sets the id of the root node.
    void root_id(SyntaxNodeId id);

    /// Constructs a new node with the given span of children
    /// and an autogenerated id.
    SyntaxNodeId make(Span<const SyntaxChild> children);

    NotNull<IndexMapPtr<SyntaxNode>> operator[](SyntaxNodeId id);
    NotNull<IndexMapPtr<const SyntaxNode>> operator[](SyntaxNodeId id) const;

private:
    SyntaxNodeId root_;
    IndexMap<SyntaxNode, IdMapper<SyntaxNodeId>> nodes_;
};

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.ast_gen import SyntaxChild
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

#endif // TIRO_COMPILER_AST_GEN_SYNTAX_TREE_HPP