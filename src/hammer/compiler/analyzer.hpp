#ifndef HAMMER_COMPILER_ANALYZER_HPP
#define HAMMER_COMPILER_ANALYZER_HPP

#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/fwd.hpp"
#include "hammer/compiler/string_table.hpp"
#include "hammer/compiler/syntax/ast.hpp"

namespace hammer::compiler {

inline bool can_use_as_value(const NodePtr<Expr>& expr) {
    return can_use_as_value(expr->expr_type());
}

class Analyzer final {
public:
    explicit Analyzer(const NodePtr<Root>& root, SymbolTable& symbols,
        StringTable& strings, Diagnostics& diag);

    void analyze();

private:
    void build_scopes(const NodePtr<>& node);
    void resolve_symbols(const NodePtr<>& node);
    void resolve_types(const NodePtr<>& root);
    void check_structure(const NodePtr<>& node);

private:
    NodePtr<Root> root_;
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;

    ScopePtr global_scope_ = nullptr;
    ScopePtr file_scope_ = nullptr;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_ANALYZER_HPP
