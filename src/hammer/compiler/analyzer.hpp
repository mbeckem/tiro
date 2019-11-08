#ifndef HAMMER_COMPILER_ANALYZER_HPP
#define HAMMER_COMPILER_ANALYZER_HPP

#include "hammer/ast/node.hpp"
#include "hammer/ast/scope.hpp"
#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/string_table.hpp"

namespace hammer {

class Analyzer final {
public:
    Analyzer(StringTable& strings, Diagnostics& diag);

    void analyze(ast::Root* root);

    // Casts the node to a scope if it is one.
    static ast::Scope* as_scope(ast::Node& node);

private:
    void build_scopes(ast::Node* node, ast::Scope* current_scope);
    void resolve_symbols(ast::Node* node);
    void resolve_var(ast::VarExpr* var);
    void resolve_types(ast::Root* root);
    void check_structure(ast::Node* node);

    // Finds the function that contains the node, or returns null if there is none.
    ast::FuncDecl* surrounding_function(ast::Node* node) const;

private:
    StringTable& strings_;
    Diagnostics& diag_;

    ast::Scope* global_scope_ = nullptr;
    ast::Scope* file_scope_ = nullptr;
};

} // namespace hammer

#endif // HAMMER_COMPILER_ANALYZER_HPP
