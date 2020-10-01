#include "support/test_context.hpp"

#include "common/format.hpp"
#include "compiler/bytecode/module.hpp"
#include "compiler/compiler.hpp"
#include "vm/module_registry.hpp"
#include "vm/modules/modules.hpp"
#include "vm/objects/module.hpp"
#include "vm/objects/string.hpp"

#include "support/test_compiler.hpp"

namespace tiro::vm {

TestContext::TestContext(std::string_view source)
    : context_(std::make_unique<Context>())
    , compiled_(test_compile_result(source))
    , module_(*context_, Nullable<Module>()) {

    Scope sc(ctx());
    Local std = sc.local(create_std_module(ctx()));

    if (!ctx().modules().add_module(ctx(), std)) {
        TIRO_ERROR("Failed to register std module.");
    }

    module_.set(load_module(ctx(), *compiled_.module));
    context_->modules().resolve_module(ctx(), module_.must_cast<Module>());
}

TestHandle<Value>
TestContext::run(std::string_view function_name, std::initializer_list<Handle<Value>> arguments) {
    return run(function_name, Span(arguments.begin(), arguments.size()));
}

TestHandle<Value>
TestContext::run(std::string_view function_name, Span<const Handle<Value>> arguments) {
    Scope sc(ctx());
    Local func = sc.local(get_export_impl(module_.must_cast<Module>(), function_name));
    Local args = sc.local<Nullable<Tuple>>();

    if (arguments.size() > 0) {
        args.set(Tuple::make(ctx(), arguments.size()));

        size_t i = 0;
        for (const auto& arg_handle : arguments) {
            args.must_cast<Tuple>()->set(i, *arg_handle);
            ++i;
        }
    }

    if (func->is_null()) {
        TIRO_ERROR("Failed to find function {} in module.", function_name);
    }

    return TestHandle<Value>(ctx(), ctx().run_init(func, maybe_null(args)));
}

TestHandle<Value> TestContext::get_export(std::string_view function_name) {
    return TestHandle<Value>(ctx(), get_export_impl(module_.must_cast<Module>(), function_name));
}

std::string TestContext::disassemble_ir() {
    return compiled_.ir.value();
}

std::string TestContext::disassemble() {
    return compiled_.bytecode.value();
}

TestHandle<Value> TestContext::make_null() {
    return TestHandle<Value>(ctx(), Null());
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

    Scope sc(ctx());
    Local name_symbol = sc.local(ctx().get_symbol(name));
    if (auto found = module->find_exported(*name_symbol)) {
        return *found;
    }
    return Null();
}

TestHandle<Value> TestCaller::run() {
    const size_t n = args_.size();

    std::vector<Handle<Value>> handle_args;
    handle_args.reserve(n);

    for (size_t i = 0; i < n; ++i)
        handle_args.push_back(args_[i]);

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
    CAPTURE(to_string(handle->type()));
    REQUIRE(handle->type() == ValueType::Null);
}

void require_bool(Handle<Value> handle, bool expected) {
    CAPTURE(to_string(handle->type()));
    REQUIRE(handle->type() == ValueType::Boolean);
    REQUIRE(handle.must_cast<Boolean>()->value() == expected);
}

void require_int(Handle<Value> handle, i64 expected) {
    CAPTURE(to_string(handle->type()));

    auto int_value = try_extract_integer(handle.get());
    REQUIRE(int_value.has_value());
    REQUIRE(*int_value == expected);
}

void require_float(Handle<Value> handle, f64 expected) {
    CAPTURE(to_string(handle->type()));
    REQUIRE(handle->type() == ValueType::Float);
    REQUIRE(handle.must_cast<Float>()->value() == expected);
}

void require_string(Handle<Value> handle, std::string_view expected) {
    CAPTURE(to_string(handle->type()));
    REQUIRE(handle->type() == ValueType::String);
    REQUIRE(handle.must_cast<String>()->view() == expected);
}

} // namespace tiro::vm
