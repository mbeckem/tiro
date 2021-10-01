#include "support/test_compiler.hpp"

namespace tiro::test_support {

static CompilerResult
test_compile_impl(std::string_view module_name, std::string_view source, bool details) {
    CompilerOptions opts;
    opts.keep_ast = details;
    opts.keep_bytecode = details;
    opts.keep_ir = details;

    Compiler compiler(std::string(module_name), opts);
    compiler.add_file("test", std::string(source));

    auto report = [&]() {
        fmt::memory_buffer buf;
        fmt::format_to(
            std::back_inserter(buf), "Failed to compile test source without errors or warnings:\n");
        for (const auto& msg : compiler.diag().messages()) {
            CursorPosition pos = compiler.cursor_pos(msg.range);
            fmt::format_to(
                std::back_inserter(buf), "  [{}:{}]: {}\n", pos.line(), pos.column(), msg.text);
        }

        TIRO_ERROR("{}", to_string(buf));
    };

    auto result = compiler.run();
    if (!result.success)
        report();

    TIRO_CHECK(result.module, "Module must have been compiled.");
    return result;
}

CompilerResult compile_result(std::string_view source, std::string_view module_name) {
    return test_compile_impl(module_name, source, true);
}

std::unique_ptr<BytecodeModule> compile(std::string_view source, std::string_view module_name) {
    return test_compile_impl(module_name, source, true).module;
}

} // namespace tiro::test_support
