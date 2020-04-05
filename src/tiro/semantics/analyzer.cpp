#include "tiro/semantics/analyzer.hpp"

#include "tiro/semantics/scope_builder.hpp"
#include "tiro/semantics/semantic_checker.hpp"
#include "tiro/semantics/simplifier.hpp"
#include "tiro/semantics/symbol_resolver.hpp"
#include "tiro/semantics/symbol_table.hpp"
#include "tiro/semantics/type_analyzer.hpp"

namespace tiro {

void visit_vars(NotNull<Binding*> binding, FunctionRef<void(VarDecl*)> v) {
    struct Helper {
        FunctionRef<void(VarDecl*)> v;

        void visit_var_binding(VarBinding* b) { v(b->var()); }

        void visit_tuple_binding(TupleBinding* b) {
            const auto& vars = b->vars();
            TIRO_DEBUG_NOT_NULL(vars);

            for (auto var : vars->entries())
                v(var);
        }
    } helper{v};
    visit(binding, helper);
}

Analyzer::Analyzer(
    SymbolTable& symbols, StringTable& strings, Diagnostics& diag)
    : symbols_(symbols)
    , strings_(strings)
    , diag_(diag)
    , global_scope_(
          symbols_.create_scope(ScopeType::Global, nullptr, nullptr)) {}

NodePtr<Root> Analyzer::analyze(Root* unowned_root) {
    TIRO_DEBUG_NOT_NULL(unowned_root);

    NodePtr<Root> root = ref(unowned_root);
    build_scopes(root);
    resolve_symbols(root);
    resolve_types(root);
    check_structure(root);

    {
        auto new_root = simplify(root);
        root = must_cast<Root>(new_root);
    }
    return root;
}

NodePtr<> Analyzer::simplify(Node* node) {
    Simplifier simplifier(symbols_, strings_, diag_);
    return simplifier.simplify(node);
}

void Analyzer::build_scopes(Node* node) {
    ScopeBuilder builder(symbols_, strings_, diag_, global_scope_);
    builder.dispatch(node);
}

void Analyzer::resolve_symbols(Node* node) {
    SymbolResolver resolver(symbols_, strings_, diag_);
    resolver.dispatch(node);
}

void Analyzer::resolve_types(Node* node) {
    TypeAnalyzer types(diag_);
    types.dispatch(node, true);
}

void Analyzer::check_structure(Node* node) {
    SemanticChecker checker(symbols_, strings_, diag_);
    checker.check(node);
}

} // namespace tiro
