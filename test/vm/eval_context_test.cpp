#include "./eval_context.hpp"

#include "hammer/compiler/compiler.hpp"
#include "hammer/vm/builtin/modules.hpp"
#include "hammer/vm/load.hpp"
#include "hammer/vm/objects/modules.hpp"
#include "hammer/vm/objects/strings.hpp"

namespace hammer::vm {

using compiler::CursorPosition;
using compiler::Compiler;

TestContext::TestContext()
    : context_(std::make_unique<Context>()) {

    Root std(ctx(), create_std_module(ctx()));
    if (!ctx().add_module(std)) {
        HAMMER_ERROR("Failed to register std module.");
    }
}

TestHandle<Value> TestContext::compile_and_run(
    std::string_view source, std::string_view function_name) {

    Root<Module> module(ctx(), compile(source));
    Root<Function> function(ctx(), find_function(module, function_name));

    if (function->is_null()) {
        HAMMER_ERROR("Failed to find function {} in module.", function_name);
    }

    return TestHandle(ctx(), ctx().run(function.handle()));
}

Module TestContext::compile(std::string_view source) {
    Compiler compiler("Test", source);

    if (!compiler.parse() || !compiler.analyze()
        || compiler.diag().message_count() > 0) {

        fmt::memory_buffer buf;
        fmt::format_to(
            buf, "Failed to compile test source without errors or warnings:\n");
        for (const auto& msg : compiler.diag().messages()) {
            CursorPosition pos = compiler.cursor_pos(msg.source);
            fmt::format_to(
                buf, "  [{}:{}]: {}\n", pos.line(), pos.column(), msg.text);
        }

        HAMMER_ERROR("{}", to_string(buf));
    }

    auto compiled = compiler.codegen();
    return load_module(ctx(), *compiled, compiler.strings());
}

Function
TestContext::find_function(Handle<Module> module, std::string_view name) {
    Tuple members = module->members();
    const size_t size = members.size();
    for (size_t i = 0; i < size; ++i) {
        Value v = members.get(i);

        if (v.is<Function>()) {
            Function f = v.as<Function>();
            if (f.tmpl().name().view() == name) {
                return f;
            }
        }
    }

    return Function();
}

} // namespace hammer::vm
