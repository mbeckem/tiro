#ifndef HAMMER_COMPILER_SEMANTICS_SEMANTIC_CHECKER_HPP
#define HAMMER_COMPILER_SEMANTICS_SEMANTIC_CHECKER_HPP

#include "hammer/compiler/syntax/ast.hpp"

namespace hammer::compiler {

class Diagnostics;
class StringTable;
class SymbolTable;

class SemanticChecker final : public DefaultNodeVisitor<SemanticChecker> {
public:
    explicit SemanticChecker(
        SymbolTable& symbols, StringTable& strings, Diagnostics& diag);
    ~SemanticChecker();

    SemanticChecker(const SemanticChecker&) = delete;
    SemanticChecker& operator=(const SemanticChecker&) = delete;

    void check(Node* node);

    void visit_root(Root* root) HAMMER_VISITOR_OVERRIDE;
    void visit_file(File* file) HAMMER_VISITOR_OVERRIDE;
    void visit_binding(Binding* binding) HAMMER_VISITOR_OVERRIDE;
    void visit_if_expr(IfExpr* expr) HAMMER_VISITOR_OVERRIDE;
    void visit_binary_expr(BinaryExpr* expr) HAMMER_VISITOR_OVERRIDE;
    void visit_node(Node* node) HAMMER_VISITOR_OVERRIDE;

private:
    bool check_lhs_expr(Expr* expr, bool allow_tuple);
    bool check_lhs_var(VarExpr* entry);

private:
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_SEMANTICS_SEMANTIC_CHECKER_HPP
