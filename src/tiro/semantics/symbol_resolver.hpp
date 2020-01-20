#ifndef TIRO_SEMANTICS_SYMBOL_RESOLVER_HPP
#define TIRO_SEMANTICS_SYMBOL_RESOLVER_HPP

#include "tiro/syntax/ast.hpp"

namespace tiro::compiler {

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

    void dispatch(Node* node);

    void visit_binding(Binding* binding) TIRO_VISITOR_OVERRIDE;
    void visit_decl(Decl* decl) TIRO_VISITOR_OVERRIDE;
    void visit_file(File* file) TIRO_VISITOR_OVERRIDE;
    void visit_var_expr(VarExpr* expr) TIRO_VISITOR_OVERRIDE;
    void visit_node(Node* node) TIRO_VISITOR_OVERRIDE;

private:
    void activate(Decl* decl);

    void dispatch_children(Node* node);

private:
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace tiro::compiler

#endif // TIRO_SEMANTICS_SYMBOL_RESOLVER_HPP
