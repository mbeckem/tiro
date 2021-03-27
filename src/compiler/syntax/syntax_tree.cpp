#include "compiler/syntax/syntax_tree.hpp"

namespace tiro {

/* [[[cog
    from codegen.unions import implement
    from codegen.syntax import SyntaxChild
    implement(SyntaxChild.tag)
]]] */
std::string_view to_string(SyntaxChildType type) {
    switch (type) {
    case SyntaxChildType::Token:
        return "Token";
    case SyntaxChildType::NodeId:
        return "NodeId";
    }
    TIRO_UNREACHABLE("Invalid SyntaxChildType.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.syntax import SyntaxChild
    implement(SyntaxChild)
]]] */
SyntaxChild SyntaxChild::make_token(const Token& token) {
    return token;
}

SyntaxChild SyntaxChild::make_node_id(const NodeId& node_id) {
    return node_id;
}

SyntaxChild::SyntaxChild(Token token)
    : type_(SyntaxChildType::Token)
    , token_(std::move(token)) {}

SyntaxChild::SyntaxChild(NodeId node_id)
    : type_(SyntaxChildType::NodeId)
    , node_id_(std::move(node_id)) {}

const SyntaxChild::Token& SyntaxChild::as_token() const {
    TIRO_DEBUG_ASSERT(
        type_ == SyntaxChildType::Token, "Bad member access on SyntaxChild: not a Token.");
    return token_;
}

const SyntaxChild::NodeId& SyntaxChild::as_node_id() const {
    TIRO_DEBUG_ASSERT(
        type_ == SyntaxChildType::NodeId, "Bad member access on SyntaxChild: not a NodeId.");
    return node_id_;
}

void SyntaxChild::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_token([[maybe_unused]] const Token& token) { stream.format("{}", token); }

        void visit_node_id([[maybe_unused]] const NodeId& node_id) { stream.format("{}", node_id); }
    };
    visit(FormatVisitor{stream});
}

bool operator==(const SyntaxChild& lhs, const SyntaxChild& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const SyntaxChild& rhs;

        bool visit_token([[maybe_unused]] const SyntaxChild::Token& token) {
            [[maybe_unused]] const auto& other = rhs.as_token();
            return token == other;
        }

        bool visit_node_id([[maybe_unused]] const SyntaxChild::NodeId& node_id) {
            [[maybe_unused]] const auto& other = rhs.as_node_id();
            return node_id == other;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const SyntaxChild& lhs, const SyntaxChild& rhs) {
    return !(lhs == rhs);
}
// [[[end]]]

SyntaxNode::SyntaxNode(SyntaxType type, SourceRange range, bool has_error, ChildStorage&& children)
    : type_(type)
    , parent_()
    , range_(range)
    , has_error_(has_error)
    , children_(std::move(children)) {}

SyntaxNode::~SyntaxNode() {}

SyntaxNode::SyntaxNode(SyntaxNode&&) noexcept = default;

SyntaxNode& SyntaxNode::operator=(SyntaxNode&& other) noexcept {
    // NOTE: absl does not declare move assign as noexcept: https://github.com/abseil/abseil-cpp/issues/325
    children_ = std::move(other.children_);
    return *this;
}

SyntaxTree::SyntaxTree(std::string_view source)
    : source_(source) {}

SyntaxTree::~SyntaxTree() {}

SyntaxTree::SyntaxTree(SyntaxTree&&) noexcept = default;

SyntaxTree& SyntaxTree::operator=(SyntaxTree&&) noexcept = default;

std::string_view SyntaxTree::source() const {
    return source_;
}

SyntaxNodeId SyntaxTree::root_id() const {
    return root_;
}

void SyntaxTree::root_id(SyntaxNodeId id) {
    root_ = id;
}

SyntaxNodeId SyntaxTree::make(SyntaxNode&& node) {
    return nodes_.emplace_back(std::move(node));
}

} // namespace tiro
