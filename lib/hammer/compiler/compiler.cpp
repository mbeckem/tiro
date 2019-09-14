#include "hammer/compiler/compiler.hpp"

#include "hammer/ast/file.hpp"
#include "hammer/compiler/analyzer.hpp"
#include "hammer/compiler/codegen.hpp"
#include "hammer/compiler/parser.hpp"

namespace hammer {

Compiler::Compiler(std::string_view file_name, std::string_view file_content)
    : strings_()
    , file_name_(file_name)
    , file_content_(file_content)
    , source_map_(strings_.insert(file_name), file_content)
    , diag_() {}

const ast::Root& Compiler::ast_root() const {
    HAMMER_CHECK(parsed_,
        "Cannot return the ast before parsing completed successfully.");
    HAMMER_ASSERT(root_, "Root must be set after parsing was done.");
    return *root_;
}

void Compiler::parse() {
    if (parsed_) {
        HAMMER_ERROR("Parse step was already executed.");
    }

    Parser parser(file_name_, file_content_, strings_, diag_);
    Parser::Result file = parser.parse_file();
    HAMMER_CHECK(file.has_node(), "Parser failed to produce a file object.");

    root_ = std::make_unique<ast::Root>();
    root_->child(file.take_node());
    parsed_ = true;
}

void Compiler::analyze() {
    if (!parsed_) {
        HAMMER_ERROR("Parse step must be executed before calling analyze().");
    }

    Analyzer analyzer(strings_, diag_);
    analyzer.analyze(root_.get());
    analyzed_ = true;
}

std::unique_ptr<CompiledModule> Compiler::codegen() {
    if (!parsed_ || !analyzed_) {
        HAMMER_ERROR(
            "Parse and analyze steps must be executed before calling "
            "codegen().");
    }

    // TODO think about root / file child relationship.
    ModuleCodegen codegen(*root_->child(), strings_, diag_);
    codegen.compile();

    auto module = codegen.take_result();
    HAMMER_CHECK(module, "Code generator did not return a valid result.");
    return module;
}

CursorPosition Compiler::cursor_pos(const SourceReference& ref) const {
    return source_map_.cursor_pos(ref);
}

} // namespace hammer
