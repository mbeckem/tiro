#ifndef HAMMER_COMPILER_COMPILER_HPP
#define HAMMER_COMPILER_COMPILER_HPP

#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/output.hpp"
#include "hammer/compiler/semantics/symbol_table.hpp"
#include "hammer/compiler/source_map.hpp"
#include "hammer/compiler/syntax/ast.hpp"
#include "hammer/core/defs.hpp"

namespace hammer::compiler {

class Compiler final {
public:
    explicit Compiler(
        std::string_view file_name, std::string_view file_content);

    const StringTable& strings() const { return strings_; }
    const Diagnostics& diag() const { return diag_; }

    const NodePtr<Root>& ast_root() const;

    bool parse();
    bool analyze();
    std::unique_ptr<CompiledModule> codegen();

    // Compute the concrete cursor position (i.e. line and column) for the given
    // source reference.
    CursorPosition cursor_pos(const SourceReference& ref) const;

private:
    StringTable strings_;

    std::string_view file_name_;
    std::string_view file_content_;
    InternedString file_name_intern_;
    SourceMap source_map_;
    Diagnostics diag_;

    // True if parsing completed. The ast may be (partially) invalid because of errors, but we can
    // still do analysis of the "good" parts.
    bool parsed_ = false;

    // True if analayze() was run. Codegen is possible if parse + analyze were executed
    // and if there were no errors reported in diag_.
    bool analyzed_ = false;

    // Set after parsing was done.
    NodePtr<Root> root_ = nullptr;

    // Popuplated by the analyzer.
    SymbolTable symbols_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_COMPILER_HPP
