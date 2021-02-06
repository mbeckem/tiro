#ifndef TIRO_COMPILER_COMPILER_HPP
#define TIRO_COMPILER_COMPILER_HPP

#include "bytecode/module.hpp"
#include "common/defs.hpp"
#include "compiler/ast/fwd.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/ir/fwd.hpp"
#include "compiler/parser/fwd.hpp"
#include "compiler/semantics/fwd.hpp"
#include "compiler/source_map.hpp"

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
    explicit Compiler(
        std::string file_name, std::string file_content, const CompilerOptions& options = {});

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
    AstPtr<AstFile> parse_file();

    std::optional<SemanticAst> analyze(NotNull<AstFile*> root);

    std::optional<ir::Module> generate_ir(const SemanticAst& ast);

    BytecodeModule generate_bytecode(ir::Module& ir_module);

private:
    CompilerOptions options_;
    StringTable strings_;

    std::string file_name_;
    std::string file_content_;
    InternedString file_name_intern_;
    SourceMap source_map_;
    Diagnostics diag_;
};

} // namespace tiro

#endif // TIRO_COMPILER_COMPILER_HPP
