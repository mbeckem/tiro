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
/// is generated. When a value is "required" (e.g. part of an expression) then it *MUST*
/// produce an actual value.
class TypeAnalyzer final : public DefaultNodeVisitor<TypeAnalyzer, bool> {
public:
    explicit TypeAnalyzer(Diagnostics& diag);
    ~TypeAnalyzer();

    TypeAnalyzer(const TypeAnalyzer&) = delete;
    TypeAnalyzer& operator=(const TypeAnalyzer&) = delete;

    void dispatch(Node* node, bool required);

    void visit_func_decl(FuncDecl* func, bool required) HAMMER_VISITOR_OVERRIDE;

    void
    visit_block_expr(BlockExpr* expr, bool required) HAMMER_VISITOR_OVERRIDE;

    void visit_if_expr(IfExpr* expr, bool required) HAMMER_VISITOR_OVERRIDE;

    void
    visit_return_expr(ReturnExpr* expr, bool required) HAMMER_VISITOR_OVERRIDE;

    void visit_expr(Expr* expr, bool required) HAMMER_VISITOR_OVERRIDE;

    void
    visit_assert_stmt(AssertStmt* stmt, bool required) HAMMER_VISITOR_OVERRIDE;

    void
    visit_while_stmt(WhileStmt* stmt, bool required) HAMMER_VISITOR_OVERRIDE;

    void visit_for_stmt(ForStmt* stmt, bool required) HAMMER_VISITOR_OVERRIDE;

    void visit_expr_stmt(ExprStmt* stmt, bool required) HAMMER_VISITOR_OVERRIDE;

    void visit_binding(Binding* binding, bool required) HAMMER_VISITOR_OVERRIDE;

    void visit_node(Node* node, bool required) HAMMER_VISITOR_OVERRIDE;

private:
    Diagnostics& diag_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_SEMANTICS_TYPE_CHECKER_HPP
