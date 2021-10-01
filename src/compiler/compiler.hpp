#ifndef TIRO_COMPILER_COMPILER_HPP
#define TIRO_COMPILER_COMPILER_HPP

#include "bytecode/module.hpp"
#include "common/defs.hpp"
#include "compiler/ast/fwd.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/ir/fwd.hpp"
#include "compiler/semantics/fwd.hpp"
#include "compiler/source_db.hpp"
#include "compiler/source_map.hpp"
#include "compiler/syntax/fwd.hpp"

#include <optional>

namespace tiro {

struct CompilerOptions {
    bool parse = true;
    bool analyze = true;
    bool compile = true;

    bool keep_cst = false;
    bool keep_ast = false;
    bool keep_ir = false;
    bool keep_bytecode = false;
};

struct CompilerResult {
    /// True if compilation completed successfully.
    bool success = false;

    /// Concrete syntax tree. Set if options.keep_cst was true
    std::optional<std::string> cst;

    /// Abstract syntax tree. Set if options.keep_ast was true
    std::optional<std::string> ast;

    /// Intermediate representation. Set if options.keep_ir was true
    std::optional<std::string> ir;

    /// Bytecode representation. If options.keep_bytecode was true
    std::optional<std::string> bytecode;

    /// Compiled module (the actual result). If options.compile was true
    std::unique_ptr<BytecodeModule> module;
};

class Compiler final {
public:
    explicit Compiler(std::string module_name, const CompilerOptions& options = {});

    void add_file(std::string filename, std::string content);

    StringTable& strings() { return strings_; }
    const StringTable& strings() const { return strings_; }

    Diagnostics& diag() { return diag_; }
    const Diagnostics& diag() const { return diag_; }
    bool has_errors() const { return diag_.has_errors(); }

    CompilerResult run();

    // Compute the concrete cursor position (i.e. line and column) for the given
    // source range.
    CursorPosition cursor_pos(const AbsoluteSourceRange& range) const;

private:
    SyntaxTree parse_file(std::string_view source);

    std::optional<SemanticAst> analyze(NotNull<AstModule*> root);

    std::optional<ir::Module> generate_ir(const SemanticAst& ast);

    BytecodeModule generate_bytecode(ir::Module& ir_module);

private:
    CompilerOptions options_;
    StringTable strings_;
    std::string module_name_;
    SourceDb sources_;
    Diagnostics diag_;
    bool started_ = false;
};

} // namespace tiro

#endif // TIRO_COMPILER_COMPILER_HPP
