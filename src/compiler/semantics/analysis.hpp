#ifndef TIRO_COMPILER_SEMANTICS_ANALYSIS_HPP
#define TIRO_COMPILER_SEMANTICS_ANALYSIS_HPP

#include "compiler/ast/fwd.hpp"
#include "compiler/ast/node.hpp"
#include "compiler/fwd.hpp"
#include "compiler/semantics/symbol_table.hpp"
#include "compiler/semantics/type_table.hpp"

namespace tiro {

class SemanticAst final {
public:
    SemanticAst(NotNull<AstFile*> root, StringTable& strings);

    SemanticAst(SemanticAst&&) noexcept = default;
    SemanticAst& operator=(SemanticAst&&) noexcept = default;

    SemanticAst(const SemanticAst&) = delete;
    SemanticAst& operator=(const SemanticAst&) = delete;

    NotNull<AstFile*> root() const { return root_; }

    const AstNodeMap& nodes() const { return nodes_; }
    AstNodeMap& nodes() { return nodes_; }

    const SymbolTable& symbols() const { return symbols_; }
    SymbolTable& symbols() { return symbols_; }

    const TypeTable& types() const { return types_; }
    TypeTable& types() { return types_; }

    StringTable& strings() const { return strings_; }

private:
    NotNull<AstFile*> root_;
    AstNodeMap nodes_;
    SymbolTable symbols_;
    TypeTable types_;
    StringTable& strings_;
};

SemanticAst analyze_ast(NotNull<AstFile*> root, StringTable& strings, Diagnostics& diag);

}; // namespace tiro

#endif // TIRO_COMPILER_SEMANTICS_ANALYSIS_HPP
