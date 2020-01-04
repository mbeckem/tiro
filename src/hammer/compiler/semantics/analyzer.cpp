#include "hammer/compiler/semantics/analyzer.hpp"

#include "hammer/compiler/semantics/scope_builder.hpp"
#include "hammer/compiler/semantics/semantic_checker.hpp"
#include "hammer/compiler/semantics/simplifier.hpp"
#include "hammer/compiler/semantics/symbol_resolver.hpp"
#include "hammer/compiler/semantics/symbol_table.hpp"
#include "hammer/compiler/semantics/type_checker.hpp"

namespace hammer::compiler {

Analyzer::Analyzer(const NodePtr<Root>& root, SymbolTable& symbols,
    StringTable& strings, Diagnostics& diag)
    : root_(root)
    , symbols_(symbols)
    , strings_(strings)
    , diag_(diag)
    , global_scope_(
          symbols_.create_scope(ScopeType::Global, nullptr, nullptr)) {}

void Analyzer::analyze() {
    HAMMER_ASSERT_NOT_NULL(root_);

    simplify(root_);
    build_scopes(root_);
    resolve_symbols(root_);
    resolve_types(root_);
    check_structure(root_);
}

void Analyzer::simplify(NodePtr<Root>& node) {
    Simplifier simplifier(strings_, diag_);

    auto result = simplifier.simplify(node);
    node = must_cast<Root>(result); // Must stay a root
}

void Analyzer::build_scopes(const NodePtr<>& node) {
    ScopeBuilder builder(symbols_, strings_, diag_, global_scope_);
    builder.dispatch(node);
}

void Analyzer::resolve_symbols(const NodePtr<>& node) {
    SymbolResolver resolver(symbols_, strings_, diag_);
    resolver.dispatch(node);
}

void Analyzer::resolve_types(const NodePtr<>& node) {
    TypeChecker checker(diag_);
    checker.check(node, false);
}

void Analyzer::check_structure(const NodePtr<>& node) {
    SemanticChecker checker(symbols_, strings_, diag_);
    checker.check(node);
}

} // namespace hammer::compiler
