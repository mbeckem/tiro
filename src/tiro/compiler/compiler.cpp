#include "tiro/compiler/compiler.hpp"

#include "tiro/codegen/module_codegen.hpp"
#include "tiro/semantics/analyzer.hpp"
#include "tiro/syntax/parser.hpp"

namespace tiro {

Compiler::Compiler(std::string_view file_name, std::string_view file_content)
    : file_name_(file_name)
    , file_content_(file_content)
    , file_name_intern_(strings_.insert(file_name_))
    , source_map_(file_name_intern_, file_content) {}

const NodePtr<Root>& Compiler::ast_root() const {
    TIRO_CHECK(parsed_,
        "Cannot return the ast before parsing completed successfully.");
    TIRO_ASSERT(root_, "Root must be set after parsing was done.");
    return root_;
}

bool Compiler::parse() {
    if (parsed_) {
        TIRO_ERROR("Parse step was already executed.");
    }

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

    parsed_ = true;
    return true;
}

bool Compiler::analyze() {
    if (!parsed_) {
        TIRO_ERROR("Parse step must be executed before calling analyze().");
    }

    if (analyzed_) {
        TIRO_ERROR("Analyze step was already executed.");
    }

    analyzed_ = true;

    Analyzer analyzer(symbols_, strings_, diag_);
    root_ = analyzer.analyze(root_);
    return !diag_.has_errors();
}

std::unique_ptr<CompiledModule> Compiler::codegen() {
    if (!parsed_ || !analyzed_) {
        TIRO_ERROR(
            "Parse and analyze steps must be executed before calling "
            "codegen().");
    }

    if (diag_.has_errors()) {
        TIRO_ERROR(
            "Cannot generate code when earlier compilation steps produced "
            "errors.");
    }

    // TODO module names.
    ModuleCodegen codegen(
        file_name_intern_, TIRO_NN(root_), symbols_, strings_, diag_);
    codegen.compile();

    auto module = codegen.take_result();
    TIRO_CHECK(module, "Code generator did not return a valid result.");
    return module;
}

CursorPosition Compiler::cursor_pos(const SourceReference& ref) const {
    return source_map_.cursor_pos(ref);
}

} // namespace tiro
