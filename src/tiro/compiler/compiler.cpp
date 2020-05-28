#include "tiro/compiler/compiler.hpp"

#include "tiro/ast/dump.hpp"
#include "tiro/bytecode_gen/gen_module.hpp"
#include "tiro/ir/module.hpp"
#include "tiro/ir_gen/gen_module.hpp"
#include "tiro/parser/parser.hpp"
#include "tiro/semantics/structure_check.hpp"
#include "tiro/semantics/symbol_resolution.hpp"
#include "tiro/semantics/symbol_table.hpp"
#include "tiro/semantics/type_check.hpp"
#include "tiro/semantics/type_table.hpp"

namespace tiro {

Compiler::Compiler(std::string_view file_name, std::string_view file_content,
    const CompilerOptions& options)
    : options_(options)
    , file_name_(file_name)
    , file_content_(file_content)
    , file_name_intern_(strings_.insert(file_name_))
    , source_map_(file_name_intern_, file_content) {}

CompilerResult Compiler::run() {
    CompilerResult result;

    if (!options_.parse) {
        result.success = true;
        return result;
    }

    auto ir_module = [&]() -> std::optional<Module> {
        auto file = parse_file();
        if (!file)
            return {};

        if (options_.keep_ast)
            result.ast = dump(file.get(), strings_);

        if (!options_.analyze) {
            result.success = true;
            return {};
        }

        TypeTable types;
        SymbolTable symbols;
        const bool analyze_ok = analyze(file, symbols, types);
        // TODO: Output semantic information together with the AST.

        if (!analyze_ok)
            return {};

        if (!options_.compile) {
            result.success = true;
            return {};
        }

        auto module = generate_ir(TIRO_NN(file.get()), symbols);
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
        dump_module(module, stream);
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
        diag_.reportf(
            Diagnostics::Error, ref, "The file contains invalid utf8.");
        return nullptr;
    }

    Parser parser(file_name_, file_content_, strings_, diag_);
    ParseResult file = parser.parse_file();
    TIRO_CHECK(file.has_node(), "Parser failed to produce a file object.");

    // TODO Multi-file
    return file.take_node();
}

bool Compiler::analyze(
    AstPtr<AstFile>& file, SymbolTable& symbols, TypeTable& types) {
    if (has_errors())
        return false;

    NotNull<AstFile*> node = TIRO_NN(file.get());

    symbols = resolve_symbols(node, strings_, diag_);
    if (has_errors())
        return false;

    types = check_types(node, diag_);
    if (has_errors())
        return false;

    check_structure(node, symbols, strings_, diag_);
    return !has_errors();
}

std::optional<Module>
Compiler::generate_ir(NotNull<AstFile*> file, const SymbolTable& symbols) {
    TIRO_DEBUG_ASSERT(!has_errors(),
        "Must not generate mir when the program already has errors.");

    // TODO ugly interface
    Module ir(file_name_intern_, strings_);
    ModuleIRGen ctx(file, ir, symbols, strings_, diag_);
    ctx.compile_module();
    if (has_errors())
        return {};

    return std::optional(std::move(ir));
}

BytecodeModule Compiler::generate_bytecode(Module& ir_module) {
    TIRO_DEBUG_ASSERT(!has_errors(),
        "Must not generate bytecode when the program already has errors.");
    return compile_module(ir_module);
}

} // namespace tiro
