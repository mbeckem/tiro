#ifndef HAMMER_COMPILER_SEMANTICS_SYMBOL_RESOLVER_HPP
#define HAMMER_COMPILER_SEMANTICS_SYMBOL_RESOLVER_HPP

#include "hammer/compiler/syntax/ast.hpp"

namespace hammer::compiler {

class Diagnostics;
class StringTable;
class SymbolTable;

class SymbolResolver final : public DefaultNodeVisitor<SymbolResolver> {
public:
    explicit SymbolResolver(
        SymbolTable& symbols, StringTable& strings, Diagnostics& diag);
    ~SymbolResolver();

    SymbolResolver(const SymbolResolver&) = delete;
    SymbolResolver& operator=(const SymbolResolver&) = delete;

    void dispatch(const NodePtr<>& node);

    void visit_decl(const NodePtr<Decl>& decl) HAMMER_VISITOR_OVERRIDE;
    void visit_file(const NodePtr<File>& file) HAMMER_VISITOR_OVERRIDE;
    void visit_var_expr(const NodePtr<VarExpr>& expr) HAMMER_VISITOR_OVERRIDE;
    void visit_node(const NodePtr<>& node) HAMMER_VISITOR_OVERRIDE;

private:
    void activate(const NodePtr<Decl>& decl);

    void dispatch_children(const NodePtr<>& node);

private:
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_SEMANTICS_SYMBOL_RESOLVER_HPP
