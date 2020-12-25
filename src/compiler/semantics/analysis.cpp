#include "compiler/semantics/analysis.hpp"

#include "compiler/ast/stmt.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/semantics/structure_check.hpp"
#include "compiler/semantics/symbol_resolution.hpp"
#include "compiler/semantics/type_check.hpp"

namespace tiro {

SemanticAst::SemanticAst(NotNull<AstFile*> root, StringTable& strings)
    : root_(root)
    , strings_(strings) {
    nodes_.register_tree(root_);
}

SemanticAst analyze_ast(NotNull<AstFile*> root, StringTable& strings, Diagnostics& diag) {
    TIRO_DEBUG_ASSERT(!diag.has_errors(), "Must not be in error state.");

    SemanticAst ast(root, strings);

    resolve_symbols(ast, diag);
    if (diag.has_errors())
        return ast;

    check_types(ast.root(), ast.types(), diag);
    if (diag.has_errors())
        return ast;

    check_structure(ast, diag);
    if (diag.has_errors())
        return ast;

    return ast;
}

} // namespace tiro
