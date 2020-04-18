#ifndef TIRO_COMPILER_COMPILER_HPP
#define TIRO_COMPILER_COMPILER_HPP

#include "tiro/bytecode/module.hpp"
#include "tiro/compiler/diagnostics.hpp"
#include "tiro/compiler/source_map.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/ir/fwd.hpp"
#include "tiro/semantics/symbol_table.hpp"
#include "tiro/syntax/ast.hpp"

#include <optional>

namespace tiro {

struct CompilerOptions {
    bool parse = true;
    bool analyze = true;
    bool compile = true;

    bool keep_ast = false;
    bool keep_ir = false;
    bool keep_bytecode = false;
};

struct CompilerResult {
    bool success = false;
    std::optional<std::string> ast;         // Set if options.keep_ast was true
    std::optional<std::string> ir;          // Set if options.keep_ir was true
    std::optional<std::string> bytecode;    // If options.keep_bytecode was true
    std::unique_ptr<BytecodeModule> module; // If options.compile was true
};

class Compiler final {
public:
    explicit Compiler(std::string_view file_name, std::string_view file_content,
        const CompilerOptions& options = {});

    StringTable& strings() { return strings_; }
    const StringTable& strings() const { return strings_; }

    Diagnostics& diag() { return diag_; }
    const Diagnostics& diag() const { return diag_; }
    bool has_errors() const { return diag_.has_errors(); }

    CompilerResult run();

    // Compute the concrete cursor position (i.e. line and column) for the given
    // source reference.
    CursorPosition cursor_pos(const SourceReference& ref) const;

private:
    NodePtr<Root> parse_file();
    bool analyze(NodePtr<Root>& root, SymbolTable& symbols);

    std::optional<Module> generate_ir(NotNull<Root*> root);

    BytecodeModule generate_bytecode(Module& ir_module);

private:
    CompilerOptions options_;
    StringTable strings_;

    std::string_view file_name_;
    std::string_view file_content_;
    InternedString file_name_intern_;
    SourceMap source_map_;
    Diagnostics diag_;
};

} // namespace tiro

#endif // TIRO_COMPILER_COMPILER_HPP
