#include "./eval_context.hpp"

#include "tiro/compiler/compiler.hpp"
#include "tiro/modules/modules.hpp"
#include "tiro/objects/modules.hpp"
#include "tiro/objects/strings.hpp"
#include "tiro/vm/load.hpp"

namespace tiro::vm {

using compiler::CursorPosition;
using compiler::Compiler;

TestContext::TestContext(std::string_view source, bool disassemble)
    : context_(std::make_unique<Context>())
    , compiler_(std::make_unique<Compiler>("Test", source))
    , module_(*context_) {

    Root std(ctx(), create_std_module(ctx()));
    if (!ctx().add_module(std)) {
        TIRO_ERROR("Failed to register std module.");
    }

    compiled_ = compile();
    if (disassemble) {
        fmt::print("{}\n", this->disassemble());
    }

    module_.set(load_module(ctx(), *compiled_, compiler_->strings()));
}

TestHandle<Value> TestContext::run(std::string_view function_name,
    std::initializer_list<Handle<Value>> arguments) {
    TIRO_ASSERT(!module_->is_null(), "Invalid module.");

    Root<Function> func(ctx(), find_function(module_, function_name));
    Root<Tuple> args(ctx());

    if (arguments.size() > 0) {
        args.set(Tuple::make(ctx(), arguments.size()));

        size_t i = 0;
        for (const auto& arg_handle : arguments) {
            args->set(i, arg_handle.get());
            ++i;
        }
    }

    if (func->is_null()) {
        TIRO_ERROR("Failed to find function {} in module.", function_name);
    }

    return TestHandle(ctx(), ctx().run(func.handle(), args));
}

std::string TestContext::disassemble() {
    TIRO_ASSERT(compiler_, "No compiler instance.");
    TIRO_ASSERT(compiled_, "No compiled module.");
    return compiler::disassemble_module(*compiled_, compiler_->strings());
}

TestHandle<Value> TestContext::make_int(i64 value) {
    return TestHandle<Value>(ctx(), ctx().get_integer(value));
}

TestHandle<Value> TestContext::make_string(std::string_view value) {
    return TestHandle<Value>(ctx(), String::make(ctx(), value));
}

TestHandle<Value> TestContext::make_boolean(bool value) {
    return TestHandle<Value>(ctx(), ctx().get_boolean(value));
}

std::unique_ptr<compiler::CompiledModule> TestContext::compile() {
    if (!compiler_->parse() || !compiler_->analyze()
        || compiler_->diag().message_count() > 0) {

        fmt::memory_buffer buf;
        fmt::format_to(
            buf, "Failed to compile test source without errors or warnings:\n");
        for (const auto& msg : compiler_->diag().messages()) {
            CursorPosition pos = compiler_->cursor_pos(msg.source);
            fmt::format_to(
                buf, "  [{}:{}]: {}\n", pos.line(), pos.column(), msg.text);
        }

        TIRO_ERROR("{}", to_string(buf));
    }

    return compiler_->codegen();
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
