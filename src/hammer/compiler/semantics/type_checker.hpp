#ifndef HAMMER_COMPILER_SEMANTICS_TYPE_CHECKER_HPP
#define HAMMER_COMPILER_SEMANTICS_TYPE_CHECKER_HPP

#include "hammer/compiler/syntax/ast.hpp"

namespace hammer::compiler {

class Diagnostics;

/// Type checking is a very primitive algorithm right now. Because the language does
/// not have static types, almost any value can be used at any place. However, complexity
/// arises from the fact that BlockExprs and IfExpr may or may not return a value, so
/// we introduce an artifical "none" type for expressions that cannot be used in a value context.
///
/// The recursive tree walk assigns a value type other than None everywhere an actual value
/// is generated. If a value is required (the boolean requires_value parameter) but none
/// is generated, a compiler error is raised (analysis usually continues to report more
/// errors, but the node will be flagged as erroneous).
class TypeChecker final : public DefaultNodeVisitor<TypeChecker, bool> {
public:
    explicit TypeChecker(Diagnostics& diag);
    ~TypeChecker();

    TypeChecker(const TypeChecker&) = delete;
    TypeChecker& operator=(const TypeChecker&) = delete;

    void check(const NodePtr<>& node, bool requires_value);

    void visit_block_expr(const NodePtr<BlockExpr>& expr,
        bool requires_value) HAMMER_VISITOR_OVERRIDE;
    void visit_if_expr(const NodePtr<IfExpr>& expr,
        bool requires_value) HAMMER_VISITOR_OVERRIDE;
    void visit_return_expr(const NodePtr<ReturnExpr>& expr,
        bool requires_value) HAMMER_VISITOR_OVERRIDE;
    void visit_expr(
        const NodePtr<Expr>& expr, bool requires_value) HAMMER_VISITOR_OVERRIDE;
    void visit_assert_stmt(const NodePtr<AssertStmt>& stmt,
        bool requires_value) HAMMER_VISITOR_OVERRIDE;
    void visit_while_stmt(const NodePtr<WhileStmt>& stmt,
        bool requires_value) HAMMER_VISITOR_OVERRIDE;
    void visit_for_stmt(const NodePtr<ForStmt>& stmt,
        bool requires_value) HAMMER_VISITOR_OVERRIDE;
    void visit_expr_stmt(const NodePtr<ExprStmt>& stmt,
        bool requires_value) HAMMER_VISITOR_OVERRIDE;
    void visit_binding(const NodePtr<Binding>& binding,
        bool requires_value) HAMMER_VISITOR_OVERRIDE;
    void visit_node(
        const NodePtr<>& node, bool requires_value) HAMMER_VISITOR_OVERRIDE;

private:
    Diagnostics& diag_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_SEMANTICS_TYPE_CHECKER_HPP
