#include "compiler/syntax/build_syntax_tree.hpp"

#include "compiler/syntax/parser_event.hpp"

namespace tiro {

namespace {

struct SyntaxNodeBuilder final {
    SyntaxType type_;
    bool has_error_ = false;
    SyntaxNode::ChildStorage children_;
    u32 pos_ = 0; // End offset of the preceding token, as a fallback when a node has 0 children.

    SyntaxNodeBuilder(SyntaxType type, u32 pos)
        : type_(type)
        , pos_(pos) {}

    void add_child(SyntaxChild&& child);
    SyntaxNode build(SyntaxTree& tree);
};

class SyntaxTreeBuilder final : public ParserEventConsumer {
public:
    SyntaxTreeBuilder(std::string_view source);

    SyntaxTreeBuilder(const SyntaxTreeBuilder&) = delete;
    SyntaxTreeBuilder& operator=(const SyntaxTreeBuilder&) = delete;

    void start_root();
    void finish_root();
    SyntaxTree&& take_tree() { return std::move(tree_); }

    void start_node(SyntaxType type) override;
    void token(Token& token) override;
    void error(std::string& message) override;
    void finish_node() override;

private:
    void link_parents();

private:
    SyntaxTree tree_;
    SourceRange last_token_range_;

    // Stack of open but not yet closed nodes.
    // The first entry (the top) is the builder for the root node.
    std::vector<SyntaxNodeBuilder> nodes_;
};

} // namespace

static SourceRange child_range(const SyntaxChild& child, const SyntaxTree& tree);

SyntaxTree build_syntax_tree(std::string_view source, Span<ParserEvent> events) {
    SyntaxTreeBuilder builder(source);
    builder.start_root();
    consume_events(events, builder);
    builder.finish_root();
    return builder.take_tree();
}

SyntaxTreeBuilder::SyntaxTreeBuilder(std::string_view source)
    : tree_(source)
    , last_token_range_(0, 0) {}

void SyntaxTreeBuilder::start_root() {
    TIRO_DEBUG_ASSERT(nodes_.empty(), "Builder was already started.");
    nodes_.emplace_back(SyntaxType::Root, 0);
}

void SyntaxTreeBuilder::finish_root() {
    TIRO_DEBUG_ASSERT(nodes_.size() >= 1, "Builder was not started.");

    while (nodes_.size() > 1) {
        finish_node();
    }

    auto root_id = tree_.make(nodes_.front().build(tree_));
    tree_.root_id(root_id);
    link_parents();
}

void SyntaxTreeBuilder::start_node(SyntaxType type) {
    auto& builder = nodes_.emplace_back(type, last_token_range_.end());
    if (type == SyntaxType::Error)
        builder.has_error_ = true;
}

void SyntaxTreeBuilder::token(Token& token) {
    TIRO_DEBUG_ASSERT(!nodes_.empty(), "No open node exists for this token.");
    last_token_range_ = token.range();

    auto& builder = nodes_.back();
    builder.add_child(SyntaxChild::make_token(std::move(token)));
}

void SyntaxTreeBuilder::error(std::string& message) {
    TIRO_DEBUG_ASSERT(!nodes_.empty(), "No open node exists for this error.");

    auto& builder = nodes_.back();
    builder.has_error_ = true;

    SyntaxError error(std::move(message), SourceRange::from_offset(last_token_range_.end()));
    tree_.errors().push_back(std::move(error));
}

void SyntaxTreeBuilder::finish_node() {
    TIRO_DEBUG_ASSERT(nodes_.size() > 1, "Must not finish the root node because of an event.");
    auto child_id = tree_.make(nodes_.back().build(tree_));
    nodes_.pop_back();
    nodes_.back().add_child(child_id);
}

void SyntaxTreeBuilder::link_parents() {
    struct Item {
        SyntaxNodeId parent_id;
        SyntaxNodeId node_id;
    };

    std::vector<Item> stack;
    stack.push_back({SyntaxNodeId(), tree_.root_id()});
    while (!stack.empty()) {
        auto [parent_id, node_id] = stack.back();
        stack.pop_back();

        auto& node_data = tree_[node_id];
        node_data.parent(parent_id);

        for (const auto& child : node_data.children()) {
            if (child.type() == SyntaxChildType::NodeId) {
                stack.push_back({node_id, child.as_node_id()});
            }
        }
    }
}

void SyntaxNodeBuilder::add_child(SyntaxChild&& child) {
    children_.push_back(std::move(child));
}

SyntaxNode SyntaxNodeBuilder::build(SyntaxTree& tree) {
    SourceRange node_range;
    if (!children_.empty()) {
        auto begin = child_range(children_.front(), tree).begin();
        auto end = child_range(children_.back(), tree).end();
        node_range = SourceRange(begin, end);
    } else {
        node_range = SourceRange(pos_, pos_);
    }

    return SyntaxNode(type_, node_range, has_error_, std::move(children_));
}

SourceRange child_range(const SyntaxChild& child, const SyntaxTree& tree) {
    struct Visitor {
        const SyntaxTree& tree;

        SourceRange visit_token(const Token& token) { return token.range(); }

        SourceRange visit_node_id(SyntaxNodeId node_id) {
            const auto& node = tree[node_id];
            return node.range();
        }
    };

    return child.visit(Visitor{tree});
}

} // namespace tiro
