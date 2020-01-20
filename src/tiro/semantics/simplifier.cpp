#include "tiro/semantics/simplifier.hpp"

#include "tiro/compiler/diagnostics.hpp"
#include "tiro/compiler/string_table.hpp"

namespace tiro::compiler {

static void flatten_string_literals(Expr* node, FunctionRef<void(Expr*)> cb) {
    if (!node || node->has_error())
        return cb(node);

    const auto traverse = [&](Node* child) {
        TIRO_ASSERT(isa<Expr>(child), "Child must always be an expression.");
        flatten_string_literals(must_cast<Expr>(child), cb);
    };

    if (auto* seq = try_cast<StringSequenceExpr>(node))
        return traverse_children(seq->strings(), traverse);

    if (auto* interp = try_cast<InterpolatedStringExpr>(node))
        return traverse_children(interp->items(), traverse);

    return cb(node);
}

Simplifier::Simplifier(StringTable& strings, Diagnostics& diag)
    : strings_(strings)
    , diag_(diag) {}

Simplifier::~Simplifier() {}

NodePtr<> Simplifier::simplify(Node* root) {
    TIRO_ASSERT_NOT_NULL(root);
    TIRO_ASSERT(root_ == nullptr, "simplify() does not support recursion.");

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
    merge_strings(seq);
}

void Simplifier::visit_interpolated_string_expr(InterpolatedStringExpr* expr) {
    visit_node(expr);
    merge_strings(expr);
}

void Simplifier::merge_strings(Expr* expr) {
    // Group as many adjacent string literals as possible together into a single string.
    // If only a single string remains, replace `seq` with that string.
    auto new_strings = make_ref<ExprList>();
    new_strings->start(expr->start());

    std::string buffer;
    auto commit_buffer = [&] {
        if (buffer.size() == 0)
            return;

        auto lit = make_ref<StringLiteral>(strings_.insert(buffer));
        new_strings->append(std::move(lit));
        buffer.clear();
    };

    flatten_string_literals(expr, [&](Expr* leaf) {
        // Merge adjacent string literals.
        if (auto lit = try_cast<StringLiteral>(leaf)) {
            buffer.append(strings_.value(lit->value()));
            return;
        }

        // A normal expression.
        commit_buffer();
        new_strings->append(ref(leaf));
    });
    commit_buffer();

    // Catch the special case where all strings were empty.
    if (new_strings->size() == 0) {
        auto empty = make_ref<StringLiteral>(strings_.insert(""));
        empty->start(expr->start());
        return replace(ref(expr), std::move(empty));
    }

    // This catches the case where all strings are static (and therefore
    // could be merged).
    if (new_strings->size() == 1 && isa<StringLiteral>(new_strings->get(0))) {
        return replace(ref(expr), ref(new_strings->get(0)));
    }

    // The remaining case catches interpolated strings mixed with static strings.
    auto replacement = make_ref<InterpolatedStringExpr>();
    replacement->start(expr->start());
    replacement->items(std::move(new_strings));
    return replace(ref(expr), replacement);
}

void Simplifier::dispatch(Node* node) {
    if (node && !node->has_error()) {
        visit(node, *this);
    }
}

void Simplifier::replace(NodePtr<> old_node, NodePtr<> new_node) {
    if (!parent_) {
        TIRO_ASSERT(old_node == root_, "Invalid old node.");
        root_ = std::move(new_node);
        return;
    }

    transform_children(parent_.get(), [&](Node* child) -> NodePtr<> {
        return child == old_node ? new_node : ref(child);
    });
}

} // namespace tiro::compiler
