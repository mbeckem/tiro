#include "hammer/compiler/semantics/analyzer.hpp"

#include "hammer/compiler/semantics/expr_analyzer.hpp"
#include "hammer/compiler/semantics/scope_builder.hpp"
#include "hammer/compiler/semantics/semantic_checker.hpp"
#include "hammer/compiler/semantics/simplifier.hpp"
#include "hammer/compiler/semantics/symbol_resolver.hpp"
#include "hammer/compiler/semantics/symbol_table.hpp"
#include "hammer/compiler/semantics/type_analyzer.hpp"

namespace hammer::compiler {

Analyzer::Analyzer(
    SymbolTable& symbols, StringTable& strings, Diagnostics& diag)
    : symbols_(symbols)
    , strings_(strings)
    , diag_(diag)
    , global_scope_(
          symbols_.create_scope(ScopeType::Global, nullptr, nullptr)) {}

NodePtr<Root> Analyzer::analyze(Root* unowned_root) {
    HAMMER_ASSERT_NOT_NULL(unowned_root);

    NodePtr<Root> root = ref(unowned_root);

    {
        auto new_root = simplify(root);
        root = must_cast<Root>(new_root);
    }

    build_scopes(root);
    resolve_symbols(root);
    resolve_types(root);
    analyze_expressions(root);
    check_structure(root);
    return root;
}

NodePtr<> Analyzer::simplify(Node* node) {
    Simplifier simplifier(strings_, diag_);
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

void Analyzer::analyze_expressions(Node* node) {
    ExprAnalyzer exprs;
    exprs.dispatch(node, true);
}

void Analyzer::check_structure(Node* node) {
    SemanticChecker checker(symbols_, strings_, diag_);
    checker.check(node);
}

} // namespace hammer::compiler
