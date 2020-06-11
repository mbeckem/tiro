#include "support/test_compiler.hpp"

#include "tiro/compiler/compiler.hpp"

namespace tiro {

std::unique_ptr<BytecodeModule> test_compile(std::string_view source) {
    Compiler compiler("test", source);

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
    return std::move(result.module);
}

} // namespace tiro
