#ifndef TIRO_SEMANTICS_SIMPLIFIER_HPP
#define TIRO_SEMANTICS_SIMPLIFIER_HPP

#include "tiro/compiler/reset_value.hpp"
#include "tiro/syntax/ast.hpp"

namespace tiro::compiler {

class Diagnostics;
class StringTable;

/// The simplifier lowers the AST from a high level tree
/// that represents the parsed source code to a lower level tree
/// that is easier to compile.
///
/// The plan is to do at least constant evaluation and
/// simplification of loops here (a single "loop" node instead
/// of multiple loop variants).
class Simplifier final : public DefaultNodeVisitor<Simplifier> {
public:
    explicit Simplifier(StringTable& strings, Diagnostics& diag);
    ~Simplifier();

    Simplifier(const Simplifier&) = delete;
    Simplifier& operator=(const Simplifier&) = delete;

    NodePtr<> simplify(Node* root);

    void visit_node(Node* node) TIRO_VISITOR_OVERRIDE;
    void
    visit_string_sequence_expr(StringSequenceExpr* seq) TIRO_VISITOR_OVERRIDE;
    void visit_interpolated_string_expr(
        InterpolatedStringExpr* expr) TIRO_VISITOR_OVERRIDE;

private:
    void merge_strings(Expr* expr);

private:
    void dispatch(Node* node);
    void simplify_children(Node* parent);
    void replace(NodePtr<> old_child, NodePtr<> new_child);

private:
    NodePtr<> root_;
    NodePtr<> parent_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace tiro::compiler

#endif // TIRO_SEMANTICS_SIMPLIFIER_HPP
