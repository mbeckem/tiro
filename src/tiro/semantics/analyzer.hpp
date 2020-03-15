#ifndef TIRO_SEMANTICS_ANALYZER_HPP
#define TIRO_SEMANTICS_ANALYZER_HPP

#include "tiro/compiler/diagnostics.hpp"
#include "tiro/compiler/fwd.hpp"
#include "tiro/core/function_ref.hpp"
#include "tiro/core/string_table.hpp"
#include "tiro/syntax/ast.hpp"

namespace tiro::compiler {

/**
 * Visits all variables bound in the given binding instance (multiple
 * vars can be defined by a tuple binding).
 */
void visit_vars(NotNull<Binding*> binding, FunctionRef<void(VarDecl*)> v);

inline bool can_use_as_value(Expr* expr) {
    return can_use_as_value(expr->expr_type());
}

class Analyzer final {
public:
    explicit Analyzer(
        SymbolTable& symbols, StringTable& strings, Diagnostics& diag);

    NodePtr<Root> analyze(Root* root);

private:
    NodePtr<> simplify(Node* node);
    void build_scopes(Node* node);
    void resolve_symbols(Node* node);
    void resolve_types(Node* node);
    void analyze_expressions(Node* node);
    void check_structure(Node* node);

private:
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;

    ScopePtr global_scope_ = nullptr;
    ScopePtr file_scope_ = nullptr;
};

} // namespace tiro::compiler

#endif // TIRO_SEMANTICS_ANALYZER_HPP
