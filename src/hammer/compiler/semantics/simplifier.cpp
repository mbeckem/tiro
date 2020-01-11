#include "hammer/compiler/semantics/simplifier.hpp"

#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/string_table.hpp"

namespace hammer::compiler {

Simplifier::Simplifier(StringTable& strings, Diagnostics& diag)
    : strings_(strings)
    , diag_(diag) {}

Simplifier::~Simplifier() {}

NodePtr<> Simplifier::simplify(Node* root) {
    HAMMER_ASSERT_NOT_NULL(root);
    HAMMER_ASSERT(root_ == nullptr, "simplify() does not support recursion.");

    root_ = ref(root);
    dispatch(root_);
    return std::exchange(root_, nullptr);
}

void Simplifier::simplify_children(Node* parent) {
    NodePtr<> old_parent = std::move(parent_);
    parent_ = ref(parent);
    traverse_children(parent, [&](Node* child) { dispatch(child); });
    parent_ = std::move(old_parent);
}

void Simplifier::visit_node(Node* node) {
    simplify_children(node);
}

void Simplifier::visit_string_sequence_expr(StringSequenceExpr* seq) {
    visit_node(seq);

    std::string buffer;
    for (auto child : seq->strings()->entries()) {
        HAMMER_CHECK(isa<StringLiteral>(child),
            "Only string literals are supported in string sequences.");

        auto lit = must_cast<StringLiteral>(child);
        HAMMER_CHECK(lit->value(), "Invalid value in string literal.");

        buffer += strings_.value(lit->value());
    }

    auto node = make_ref<StringLiteral>(strings_.insert(buffer));
    replace(ref(seq), node);
}

void Simplifier::dispatch(Node* node) {
    if (node && !node->has_error()) {
        visit(node, *this);
    }
}

void Simplifier::replace(NodePtr<> old_node, NodePtr<> new_node) {
    if (!parent_) {
        HAMMER_ASSERT(old_node == root_, "Invalid old node.");
        root_ = std::move(new_node);
        return;
    }

    transform_children(parent_.get(), [&](Node* child) -> NodePtr<> {
        return child == old_node ? new_node : ref(child);
    });
}

ResetValue<NodePtr<>> Simplifier::enter(Node* new_parent) {
    auto old_parent = std::exchange(parent_, ref(new_parent));
    return {parent_, std::move(old_parent)};
}

} // namespace hammer::compiler
