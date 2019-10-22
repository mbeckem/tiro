#include "hammer/ast/node.hpp"

#include "hammer/ast/node_formatter.hpp"
#include "hammer/ast/stmt.hpp"

namespace hammer::ast {

std::string_view to_string(NodeKind kind) {
    switch (kind) {
#define HAMMER_AST_LEAF_TYPE(Type, ParentType) \
    case NodeKind::Type:                       \
        return #Type;

#include "hammer/ast/node_types.inc"
    }

    HAMMER_UNREACHABLE("Invalid node kind.");
}

Node::Node(NodeKind kind)
    : kind_(kind) {}

Node::~Node() {
    children_.clear_and_dispose([](Node* n) { delete n; });
}

void Node::dump_impl(NodeFormatter& fmt) const {
    std::string start_string;
    if (start_) {
        start_string = ::fmt::format("{} [{}:{})",
            fmt.strings().value(start_.file_name()), start_.begin(),
            start_.end());
    } else {
        start_string = "N/A";
    }

    fmt.properties("start", start_string, "has_error", has_error());
}

Node* Node::first_child() {
    if (!children_.empty()) {
        return &children_.front();
    }
    return nullptr;
}

Node* Node::last_child() {
    if (!children_.empty()) {
        return &children_.back();
    }
    return nullptr;
}

Node* Node::next_child(Node* child) {
    if (!child)
        return nullptr;

    auto pos = std::next(children_.iterator_to(*child));
    if (pos != children_.end())
        return &*pos;

    return nullptr;
}

Node* Node::prev_child(Node* child) {
    if (!child)
        return nullptr;

    auto pos = children_.iterator_to(*child);
    if (pos == children_.begin())
        return nullptr;

    --pos;
    return &*pos;
}

void Node::add_child_impl(ast::Node* child) {
    if (!child)
        return;

    HAMMER_ASSERT(child->parent() == nullptr,
        "The new child must not have an existing parent.");
    child->parent(this);
    children_.push_back(*child);
}

void Node::remove_child(Node* child) {
    if (!child)
        return;

    auto iter = children_.iterator_to(*child);
    children_.erase_and_dispose(iter, [](Node* n) { delete n; });
}

void dump(
    const Node& n, std::ostream& os, const StringTable& strings, int indent) {
    NodeFormatter fmt(strings, os, std::min(0, indent));
    fmt.visit_node(n);
}

} // namespace hammer::ast
