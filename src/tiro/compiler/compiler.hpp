#ifndef TIRO_COMPILER_COMPILER_HPP
#define TIRO_COMPILER_COMPILER_HPP

#include "tiro/bytecode/module.hpp"
#include "tiro/compiler/diagnostics.hpp"
#include "tiro/compiler/source_map.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/semantics/symbol_table.hpp"
#include "tiro/syntax/ast.hpp"

#include <optional>

namespace tiro {

class Compiler final {
public:
    explicit Compiler(
        std::string_view file_name, std::string_view file_content);

    StringTable& strings() { return strings_; }
    const StringTable& strings() const { return strings_; }

    Diagnostics& diag() { return diag_; }
    const Diagnostics& diag() const { return diag_; }
    bool has_errors() const { return diag_.has_errors(); }

    const NodePtr<Root>& ast_root() const;

    bool parse();
    bool analyze();
    std::optional<CompiledModule> codegen();

    // Compute the concrete cursor position (i.e. line and column) for the given
    // source reference.
    CursorPosition cursor_pos(const SourceReference& ref) const;

private:
    enum Stage {
        // Fresh instance.
        Ready,

        // Active after parsing.
        // The ast may be (partially) invalid because of errors, but we can
        // still do analysis of the "good" parts.
        Parsed,

        // True if analayze() was run. Codegen is possible if parse + analyze were executed
        // and if there were no errors reported in diag_.
        Analyzed,

        // After codegen.
        Generated,
    };

private:
    StringTable strings_;

    std::string_view file_name_;
    std::string_view file_content_;
    InternedString file_name_intern_;
    SourceMap source_map_;
    Diagnostics diag_;
    Stage stage_;

    // Set after parsing was done.
    NodePtr<Root> root_ = nullptr;

    // Popuplated by the analyzer.
    SymbolTable symbols_;
};

} // namespace tiro

#endif // TIRO_COMPILER_COMPILER_HPP
