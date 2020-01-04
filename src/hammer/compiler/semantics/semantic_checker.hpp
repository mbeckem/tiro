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

    void check(const NodePtr<>& node);

    void visit_root(const NodePtr<Root>& root) HAMMER_VISITOR_OVERRIDE;
    void visit_file(const NodePtr<File>& file) HAMMER_VISITOR_OVERRIDE;
    void visit_if_expr(const NodePtr<IfExpr>& expr) HAMMER_VISITOR_OVERRIDE;
    void visit_var_decl(const NodePtr<VarDecl>& var) HAMMER_VISITOR_OVERRIDE;
    void
    visit_binary_expr(const NodePtr<BinaryExpr>& expr) HAMMER_VISITOR_OVERRIDE;
    void visit_node(const NodePtr<>& node) HAMMER_VISITOR_OVERRIDE;

private:
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_SEMANTICS_SEMANTIC_CHECKER_HPP
