#ifndef HAMMER_COMPILER_SEMANTICS_SIMPLIFIER_HPP
#define HAMMER_COMPILER_SEMANTICS_SIMPLIFIER_HPP

#include "hammer/compiler/reset_value.hpp"
#include "hammer/compiler/syntax/ast.hpp"

namespace hammer::compiler {

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

    void visit_node(Node* node) HAMMER_VISITOR_OVERRIDE;
    void
    visit_string_sequence_expr(StringSequenceExpr* seq) HAMMER_VISITOR_OVERRIDE;

private:
    void dispatch(Node* node);
    void simplify_children(Node* parent);
    void replace(NodePtr<> old_child, NodePtr<> new_child);

    ResetValue<NodePtr<>> enter(Node* parent);

private:
    NodePtr<> root_;
    NodePtr<> parent_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_SEMANTICS_SIMPLIFIER_HPP
