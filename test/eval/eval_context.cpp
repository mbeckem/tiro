#include "./eval_context.hpp"

#include "tiro/bytecode/module.hpp"
#include "tiro/compiler/compiler.hpp"
#include "tiro/core/format.hpp"
#include "tiro/modules/modules.hpp"
#include "tiro/objects/modules.hpp"
#include "tiro/objects/strings.hpp"
#include "tiro/vm/load.hpp"

namespace tiro::vm {

TestContext::TestContext(std::string_view source)
    : context_(std::make_unique<Context>())
    , compiled_(compile(source))
    , module_(*context_) {

    Root std(ctx(), create_std_module(ctx()));
    if (!ctx().add_module(std)) {
        TIRO_ERROR("Failed to register std module.");
    }

    module_.set(load_module(ctx(), *compiled_));
}

TestHandle<Value>
TestContext::run(std::string_view function_name, std::initializer_list<Handle<Value>> arguments) {
    return run(function_name, Span(arguments.begin(), arguments.size()));
}

TestHandle<Value>
TestContext::run(std::string_view function_name, Span<const Handle<Value>> arguments) {
    TIRO_DEBUG_ASSERT(!module_->is_null(), "Invalid module.");

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
    TIRO_DEBUG_ASSERT(compiled_, "No compiled module.");

    StringFormatStream stream;
    dump_module(*compiled_, stream);
    return stream.take_str();
}

TestHandle<Value> TestContext::make_int(i64 value) {
    return TestHandle<Value>(ctx(), ctx().get_integer(value));
}

TestHandle<Value> TestContext::make_float(f64 value) {
    return TestHandle<Value>(ctx(), Float::make(ctx(), value));
}

TestHandle<Value> TestContext::make_string(std::string_view value) {
    return TestHandle<Value>(ctx(), String::make(ctx(), value));
}

TestHandle<Value> TestContext::make_boolean(bool value) {
    return TestHandle<Value>(ctx(), ctx().get_boolean(value));
}

std::unique_ptr<BytecodeModule> TestContext::compile(std::string_view source) {
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

    TIRO_DEBUG_ASSERT(result.module, "Module must have been compiled.");
    return std::move(result.module);
}

Function TestContext::find_function(Handle<Module> module, std::string_view name) {
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

TestHandle<Value> TestCaller::run() {
    std::vector<Handle<Value>> handle_args(args_.size());
    for (size_t i = 0, n = args_.size(); i < n; ++i)
        handle_args[i] = args_[i].handle();

    return ctx_->run(function_name_, handle_args);
}

void TestCaller::returns_null() {
    return require_null(run());
}

void TestCaller::returns_bool(bool expected) {
    return require_bool(run(), expected);
}

void TestCaller::returns_int(i64 expected) {
    return require_int(run(), expected);
}

void TestCaller::returns_float(f64 expected) {
    return require_float(run(), expected);
}

void TestCaller::returns_string(std::string_view expected) {
    return require_string(run(), expected);
}

void TestCaller::throws() {
    // TODO: This should be a special runtime error eventually
    REQUIRE_THROWS_AS(run(), Error);
}

void require_null(Handle<Value> handle) {
    REQUIRE(handle->type() == ValueType::Null);
}

void require_bool(Handle<Value> handle, bool expected) {
    REQUIRE(handle->type() == ValueType::Boolean);
    REQUIRE(handle.strict_cast<Boolean>()->value() == expected);
}

void require_int(Handle<Value> handle, i64 expected) {
    auto int_value = try_extract_integer(handle.get());
    REQUIRE(int_value.has_value());
    REQUIRE(*int_value == expected);
}

void require_float(Handle<Value> handle, f64 expected) {
    REQUIRE(handle->type() == ValueType::Float);
    REQUIRE(handle.strict_cast<Float>()->value() == expected);
}

void require_string(Handle<Value> handle, std::string_view expected) {
    REQUIRE(handle->type() == ValueType::String);
    REQUIRE(handle.strict_cast<String>()->view() == expected);
}

} // namespace tiro::vm
