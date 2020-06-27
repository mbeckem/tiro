#include "support/test_context.hpp"

#include "common/format.hpp"
#include "compiler/bytecode/module.hpp"
#include "compiler/compiler.hpp"
#include "vm/load.hpp"
#include "vm/modules/modules.hpp"
#include "vm/objects/module.hpp"
#include "vm/objects/string.hpp"

#include "support/test_compiler.hpp"

namespace tiro::vm {

TestContext::TestContext(std::string_view source)
    : context_(std::make_unique<Context>())
    , compiled_(test_compile_result(source))
    , module_(*context_) {

    Root std(ctx(), create_std_module(ctx()));
    if (!ctx().add_module(std)) {
        TIRO_ERROR("Failed to register std module.");
    }

    module_.set(load_module(ctx(), *compiled_.module));
}

TestHandle<Value>
TestContext::run(std::string_view function_name, std::initializer_list<Handle<Value>> arguments) {
    return run(function_name, Span(arguments.begin(), arguments.size()));
}

TestHandle<Value>
TestContext::run(std::string_view function_name, Span<const Handle<Value>> arguments) {
    Root<Value> func(ctx(), get_export_impl(module_, function_name));
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

TestHandle<Value> TestContext::get_export(std::string_view function_name) {
    return TestHandle<Value>(ctx(), get_export_impl(module_, function_name));
}

std::string TestContext::disassemble_ir() {
    return compiled_.ir.value();
}

std::string TestContext::disassemble() {
    return compiled_.bytecode.value();
}

TestHandle<Value> TestContext::make_null() {
    return TestHandle<Value>(ctx(), Null::make(ctx()));
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

TestHandle<Value> TestContext::make_symbol(std::string_view value) {
    return TestHandle<Value>(ctx(), ctx().get_symbol(value));
}

TestHandle<Value> TestContext::make_boolean(bool value) {
    return TestHandle<Value>(ctx(), ctx().get_boolean(value));
}

Value TestContext::get_export_impl(Handle<Module> module, std::string_view name) {
    TIRO_DEBUG_ASSERT(!module->is_null(), "Invalid module.");

    vm::Root<vm::Symbol> vm_name(ctx(), ctx().get_symbol(name));
    if (auto found = module->find_exported(vm_name)) {
        return *found;
    }
    return Null();
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
