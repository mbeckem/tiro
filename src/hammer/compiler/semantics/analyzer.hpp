#ifndef HAMMER_COMPILER_SEMANTICS_ANALYZER_HPP
#define HAMMER_COMPILER_SEMANTICS_ANALYZER_HPP

#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/fwd.hpp"
#include "hammer/compiler/string_table.hpp"
#include "hammer/compiler/syntax/ast.hpp"

namespace hammer::compiler {

template<typename Visitor>
void visit_vars(Binding* binding, Visitor&& v) {
    struct Helper {
        Visitor* v;

        void visit_var_binding(VarBinding* b) { (*v)(b->var()); }

        void visit_tuple_binding(TupleBinding* b) {
            const auto& vars = b->vars();
            HAMMER_ASSERT_NOT_NULL(vars);

            for (auto var : vars->entries())
                (*v)(var);
        }
    } helper{std::addressof(v)};
    visit(binding, helper);
}

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

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_SEMANTICS_ANALYZER_HPP
