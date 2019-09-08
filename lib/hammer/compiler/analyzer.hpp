#ifndef HAMMER_COMPILER_ANALYZER_HPP
#define HAMMER_COMPILER_ANALYZER_HPP

#include "hammer/ast/node.hpp"
#include "hammer/ast/scope.hpp"
#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/string_table.hpp"

namespace hammer {

class Analyzer {
public:
    Analyzer(StringTable& strings, Diagnostics& diag);

    void analyze(ast::Root* Root);

    // Casts the node to a scope if it is one.
    static ast::Scope* as_scope(ast::Node& node);

private:
    void build_scopes(ast::Node* node, ast::Scope* current_scope);
    void resolve_symbols(ast::Node* node);
    void resolve_var(ast::VarExpr* var);
    void resolve_types(ast::Root* root);
    void check_structure(ast::Node* node);

private:
    StringTable& strings_;
    Diagnostics& diag_;

    ast::Scope* global_scope_ = nullptr;
    ast::Scope* file_scope_ = nullptr;
};

} // namespace hammer

#endif // HAMMER_COMPILER_ANALYZER_HPP
