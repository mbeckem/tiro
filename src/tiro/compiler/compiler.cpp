#include "tiro/compiler/compiler.hpp"

#include "tiro/bytecode_gen/gen_module.hpp"
#include "tiro/ir/module.hpp"
#include "tiro/ir_gen/gen_module.hpp"
#include "tiro/semantics/analyzer.hpp"
#include "tiro/syntax/parser.hpp"

namespace tiro {

Compiler::Compiler(std::string_view file_name, std::string_view file_content)
    : file_name_(file_name)
    , file_content_(file_content)
    , file_name_intern_(strings_.insert(file_name_))
    , source_map_(file_name_intern_, file_content)
    , stage_(Ready) {}

const NodePtr<Root>& Compiler::ast_root() const {
    TIRO_CHECK(stage_ >= Parsed,
        "Cannot return the ast before parsing completed successfully.");
    TIRO_DEBUG_ASSERT(root_, "Root must be set after parsing was done.");
    return root_;
}

bool Compiler::parse() {
    TIRO_CHECK(stage_ < Parsed, "Parse step was already executed.");
    stage_ = Parsed;

    if (auto res = validate_utf8(file_content_); !res.ok) {
        SourceReference ref = SourceReference::from_std_offsets(
            file_name_intern_, res.error_offset, res.error_offset + 1);
        diag_.reportf(
            Diagnostics::Error, ref, "The file contains invalid utf8.");
        return false;
    }

    Parser parser(file_name_, file_content_, strings_, diag_);
    ParseResult file = parser.parse_file();
    TIRO_CHECK(file.has_node(), "Parser failed to produce a file object.");

    root_ = make_ref<Root>();
    root_->file(file.take_node());
    return true;
}

bool Compiler::analyze() {
    TIRO_CHECK(stage_ < Analyzed, "Analyze step was already executed.");
    TIRO_CHECK(stage_ >= Parsed,
        "Parse step must be executed before calling analyze().");
    stage_ = Analyzed;

    Analyzer analyzer(symbols_, strings_, diag_);
    root_ = analyzer.analyze(root_);
    return !has_errors();
}

std::optional<BytecodeModule> Compiler::codegen() {
    TIRO_CHECK(stage_ < Generated, "Codegen step was already executed.");
    TIRO_CHECK(stage_ >= Analyzed,
        "Parse and analyze steps must be executed before calling "
        "codegen().");
    stage_ = Generated;

    TIRO_CHECK(!has_errors(),
        "Cannot generate code when earlier compilation steps produced "
        "errors.");

    Module ir_module(file_name_intern_, strings_);
    {
        ModuleIRGen ctx(TIRO_NN(root_), ir_module, diag_, strings_);
        ctx.compile_module();
        if (has_errors())
            return {};
    }

    BytecodeModule bytecode_module = compile_module(ir_module);
    return {std::move(bytecode_module)};
}

CursorPosition Compiler::cursor_pos(const SourceReference& ref) const {
    return source_map_.cursor_pos(ref);
}

} // namespace tiro
