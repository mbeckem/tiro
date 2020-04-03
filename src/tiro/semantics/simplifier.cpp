#include "tiro/semantics/simplifier.hpp"

#include "tiro/compiler/diagnostics.hpp"
#include "tiro/core/string_table.hpp"
#include "tiro/semantics/symbol_table.hpp"

namespace tiro {

static void flatten_string_literals(Expr* node, FunctionRef<void(Expr*)> cb) {
    if (!node || node->has_error())
        return cb(node);

    const auto traverse = [&](Node* child) {
        TIRO_ASSERT(isa<Expr>(child), "Child must always be an expression.");
        flatten_string_literals(must_cast<Expr>(child), cb);
    };

    if (auto* seq = try_cast<StringSequenceExpr>(node))
        return traverse_children(TIRO_NN(seq->strings()), traverse);

    if (auto* interp = try_cast<InterpolatedStringExpr>(node))
        return traverse_children(TIRO_NN(interp->items()), traverse);

    return cb(node);
}

template<typename E, typename... Args>
Ref<E> make_expr(ScopePtr scope, ExprType type, Args&&... args) {
    auto ref = make_ref<E>(std::forward<Args>(args)...);
    ref->surrounding_scope(std::move(scope));
    ref->expr_type(type);
    return ref;
}

Simplifier::Simplifier(
    SymbolTable& symbols, StringTable& strings, Diagnostics& diag)
    : symbols_(symbols)
    , strings_(strings)
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
    traverse_children(TIRO_NN(parent), [&](Node* child) { dispatch(child); });
    parent_ = std::move(old_parent);
}

void Simplifier::visit_node(Node* node) {
    simplify_children(node);
}

void Simplifier::visit_binary_expr(BinaryExpr* expr) {
    simplify_children(expr);

    const auto replace_binary = [&](BinaryOperator op) {
        auto new_rhs = make_expr<BinaryExpr>(
            expr->surrounding_scope(), ExprType::Value, op);
        new_rhs->start(expr->start());
        new_rhs->left(expr->left());
        new_rhs->right(expr->right());

        expr->operation(BinaryOperator::Assign);
        expr->right(new_rhs);
    };

    switch (expr->operation()) {
    case BinaryOperator::AssignPlus:
        return replace_binary(BinaryOperator::Plus);
    case BinaryOperator::AssignMinus:
        return replace_binary(BinaryOperator::Minus);
    case BinaryOperator::AssignMultiply:
        return replace_binary(BinaryOperator::Multiply);
    case BinaryOperator::AssignDivide:
        return replace_binary(BinaryOperator::Divide);
    case BinaryOperator::AssignModulus:
        return replace_binary(BinaryOperator::Modulus);
    case BinaryOperator::AssignPower:
        return replace_binary(BinaryOperator::Power);
    default:
        break;
    }
}

void Simplifier::visit_string_sequence_expr(StringSequenceExpr* seq) {
    simplify_children(seq);
    merge_strings(seq);
}

void Simplifier::visit_interpolated_string_expr(InterpolatedStringExpr* expr) {
    simplify_children(expr);
    merge_strings(expr);
}

void Simplifier::merge_strings(Expr* expr) {
    // Group as many adjacent string literals as possible together into a single string.
    // If only a single string remains, replace `seq` with that string.
    auto new_strings = make_ref<ExprList>();
    new_strings->start(expr->start());

    std::string buffer;
    auto commit_buffer = [&] {
        if (buffer.empty())
            return;

        auto lit = make_expr<StringLiteral>(expr->surrounding_scope(),
            ExprType::Value, strings_.insert(buffer));
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
        auto empty = make_expr<StringLiteral>(
            expr->surrounding_scope(), expr->expr_type(), strings_.insert(""));
        empty->start(expr->start());
        return replace(ref(expr), std::move(empty));
    }

    // This catches the case where all strings are static (and therefore
    // could be merged).
    if (new_strings->size() == 1 && isa<StringLiteral>(new_strings->get(0))) {
        return replace(ref(expr), ref(new_strings->get(0)));
    }

    // The remaining case catches interpolated strings mixed with static strings.
    auto replacement = make_expr<InterpolatedStringExpr>(
        expr->surrounding_scope(), expr->expr_type());
    replacement->start(expr->start());
    replacement->items(std::move(new_strings));
    return replace(ref(expr), replacement);
}

void Simplifier::dispatch(Node* node) {
    if (node && !node->has_error()) {
        visit(TIRO_NN(node), *this);
    }
}

void Simplifier::replace(NodePtr<> old_node, NodePtr<> new_node) {
    if (!parent_) {
        TIRO_ASSERT(old_node == root_, "Invalid old node.");
        root_ = std::move(new_node);
        return;
    }

    transform_children(TIRO_NN(parent_.get()), [&](Node* child) -> NodePtr<> {
        return child == old_node ? new_node : ref(child);
    });
}

} // namespace tiro
