#include "compiler/compiler.hpp"

#include "bytecode/formatting.hpp"
#include "compiler/ast/dump.hpp"
#include "compiler/bytecode_gen/module.hpp"
#include "compiler/ir/module.hpp"
#include "compiler/ir_gen/module.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/semantics/analysis.hpp"

namespace tiro {

Compiler::Compiler(std::string file_name, std::string file_content, const CompilerOptions& options)
    : options_(options)
    , file_name_(std::move(file_name))
    , file_content_(std::move(file_content))
    , file_name_intern_(strings_.insert(file_name_))
    , source_map_(file_name_intern_, file_content_) {}

CompilerResult Compiler::run() {
    CompilerResult result;

    if (!options_.parse) {
        result.success = true;
        return result;
    }

    auto ir_module = [&]() -> std::optional<ir::Module> {
        auto file = parse_file();
        if (!file)
            return {};

        if (options_.keep_ast)
            result.ast = dump(file.get(), strings_);

        if (!options_.analyze) {
            result.success = true;
            return {};
        }

        auto semantic_ast = analyze(TIRO_NN(file.get()));
        if (!semantic_ast)
            return {};

        if (!options_.compile) {
            result.success = true;
            return {};
        }

        auto module = generate_ir(*semantic_ast);
        if (!module)
            return {};

        // Symbol table and ast no longer needed.
        return module;
    }();

    if (!ir_module)
        return result;

    if (options_.keep_ir) {
        StringFormatStream stream;
        dump_module(*ir_module, stream);
        result.ir = stream.take_str();
    }

    BytecodeModule module = generate_bytecode(*ir_module);
    if (options_.keep_bytecode) {
        // TODO: weave in debug information
        StringFormatStream stream;
        format_module(module, stream);
        result.bytecode = stream.take_str();
    }

    result.success = true;
    result.module = std::make_unique<BytecodeModule>(std::move(module));
    return result;
}

CursorPosition Compiler::cursor_pos(const SourceReference& ref) const {
    return source_map_.cursor_pos(ref);
}

AstPtr<AstFile> Compiler::parse_file() {
    if (auto res = validate_utf8(file_content_); !res.ok) {
        SourceReference ref = SourceReference::from_std_offsets(
            file_name_intern_, res.error_offset, res.error_offset + 1);
        diag_.reportf(Diagnostics::Error, ref, "The file contains invalid utf8.");
        return nullptr;
    }

    Parser parser(file_name_, file_content_, strings_, diag_);
    ParseResult file = parser.parse_file();
    TIRO_CHECK(file.has_node(), "Parser failed to produce a file object.");

    // TODO Multi-file
    return file.take_node();
}

std::optional<SemanticAst> Compiler::analyze(NotNull<AstFile*> root) {
    if (has_errors())
        return {};

    SemanticAst ast = analyze_ast(root, strings_, diag_);
    if (has_errors())
        return {};

    return ast;
}

std::optional<ir::Module> Compiler::generate_ir(const SemanticAst& ast) {
    TIRO_DEBUG_ASSERT(!has_errors(), "Must not generate mir when the program already has errors.");

    ir::Module mod(file_name_intern_, strings_);
    ir::ModuleContext ctx{file_content_, ast, diag_};
    ir::ModuleIRGen gen(ctx, mod);
    gen.compile_module();
    if (has_errors())
        return {};

    return std::optional(std::move(mod));
}

BytecodeModule Compiler::generate_bytecode(ir::Module& mod) {
    TIRO_DEBUG_ASSERT(
        !has_errors(), "Must not generate bytecode when the program already has errors.");
    return compile_module(mod);
}

} // namespace tiro
