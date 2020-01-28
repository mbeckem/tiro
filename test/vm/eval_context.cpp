#include "./eval_context.hpp"

#include "tiro/compiler/compiler.hpp"
#include "tiro/modules/modules.hpp"
#include "tiro/objects/modules.hpp"
#include "tiro/objects/strings.hpp"
#include "tiro/vm/load.hpp"

namespace tiro::vm {

using compiler::CursorPosition;
using compiler::Compiler;

TestContext::TestContext(std::string_view source)
    : context_(std::make_unique<Context>())
    , module_(*context_) {

    Root std(ctx(), create_std_module(ctx()));
    if (!ctx().add_module(std)) {
        TIRO_ERROR("Failed to register std module.");
    }

    module_.set(compile(source));
}

TestHandle<Value> TestContext::run(std::string_view function_name) {
    TIRO_ASSERT(!module_->is_null(), "Invalid module.");

    Root<Function> function(ctx(), find_function(module_, function_name));

    if (function->is_null()) {
        TIRO_ERROR("Failed to find function {} in module.", function_name);
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

        TIRO_ERROR("{}", to_string(buf));
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

} // namespace tiro::vm
