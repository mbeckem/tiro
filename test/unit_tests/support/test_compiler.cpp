#include "support/test_compiler.hpp"

namespace tiro {

static CompilerResult test_compile_impl(std::string_view source, bool details) {
    CompilerOptions opts;
    opts.keep_ast = details;
    opts.keep_bytecode = details;
    opts.keep_ir = details;

    Compiler compiler("test", source, opts);

    auto report = [&]() {
        fmt::memory_buffer buf;
        fmt::format_to(buf, "Failed to compile test source without errors or warnings:\n");
        for (const auto& msg : compiler.diag().messages()) {
            CursorPosition pos = compiler.cursor_pos(msg.source);
            fmt::format_to(buf, "  [{}:{}]: {}\n", pos.line(), pos.column(), msg.text);
        }

        TIRO_ERROR("{}", to_string(buf));
    };

    auto result = compiler.run();
    if (!result.success)
        report();

    TIRO_CHECK(result.module, "Module must have been compiled.");
    return result;
}

CompilerResult test_compile_result(std::string_view source) {
    return test_compile_impl(source, true);
}

std::unique_ptr<BytecodeModule> test_compile(std::string_view source) {
    return test_compile_impl(source, true).module;
}

} // namespace tiro
