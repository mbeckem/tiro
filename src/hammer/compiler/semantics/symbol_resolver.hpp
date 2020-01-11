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

    void dispatch(Node* node);

    void visit_binding(Binding* binding) HAMMER_VISITOR_OVERRIDE;
    void visit_decl(Decl* decl) HAMMER_VISITOR_OVERRIDE;
    void visit_file(File* file) HAMMER_VISITOR_OVERRIDE;
    void visit_var_expr(VarExpr* expr) HAMMER_VISITOR_OVERRIDE;
    void visit_node(Node* node) HAMMER_VISITOR_OVERRIDE;

private:
    void activate(Decl* decl);

    void dispatch_children(Node* node);

private:
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_SEMANTICS_SYMBOL_RESOLVER_HPP
