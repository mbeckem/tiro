#ifndef TIRO_TEST_SUPPORT_TEST_CONTEXT_HPP
#define TIRO_TEST_SUPPORT_TEST_CONTEXT_HPP

#include "common/span.hpp"
#include "compiler/compiler.hpp"
#include "vm/context.hpp"
#include "vm/handles/global.hpp"
#include "vm/handles/handle.hpp"
#include "vm/handles/scope.hpp"
#include "vm/math.hpp"

#include <catch.hpp>

#include <memory>

namespace tiro::vm {

class TestCaller;

template<typename T>
class TestHandle final : public detail::HandleOps<T, TestHandle<T>> {
public:
    template<typename U>
    TestHandle(Context& ctx, const U& initial)
        : handle_(std::make_unique<Global<T>>(ctx, initial)) {}

    auto operator->() { return handle_->operator->(); }

    operator Handle<T>() const { return handle(); }
    Handle<T> handle() const { return *handle_; }

private:
    friend SlotAccess;

    Value* get_slot() const { return tiro::vm::get_slot(*handle_); }

private:
    std::unique_ptr<Global<T>> handle_;
};

class TestContext final {
public:
    explicit TestContext(std::string_view source);

    TestHandle<Value>
    run(std::string_view function_name, std::initializer_list<Handle<Value>> arguments = {});

    // Executes a function call on the exported function with that name.
    TestHandle<Value> run(std::string_view function_name, Span<const Handle<Value>> arguments);

    // Returns the exported function with that name.
    TestHandle<Value> get_export(std::string_view function_name);

    template<typename... Args>
    inline TestCaller call(std::string_view function_name, const Args&... args);

    Context& ctx() {
        TIRO_DEBUG_ASSERT(context_, "Invalid context.");
        return *context_;
    }

    std::string disassemble_ir();
    std::string disassemble();

    TestHandle<Value> make_null();
    TestHandle<Value> make_int(i64 value);
    TestHandle<Value> make_float(f64 value);
    TestHandle<Value> make_string(std::string_view value);
    TestHandle<Value> make_symbol(std::string_view value);
    TestHandle<Value> make_boolean(bool value);

private:
    Value get_export_impl(Handle<Module> module, std::string_view name);

private:
    std::unique_ptr<Context> context_;
    CompilerResult compiled_;
    Global<Nullable<Module>> module_;
};

class TestCaller final {
public:
    explicit TestCaller(TestContext* ctx, std::string_view function_name)
        : ctx_(ctx)
        , function_name_(function_name) {}

public:
    template<typename... Args>
    TestCaller& args(const Args&... args) {
        (args_.push_back(convert_arg(args)), ...);
        return *this;
    }

    void returns_null();
    void returns_bool(bool expected);
    void returns_int(i64 expected);
    void returns_float(f64 expected);
    void returns_string(std::string_view expected);

    void throws();

    TestHandle<Value> run();

private:
    TestHandle<Value> convert_arg(std::nullptr_t) { return ctx_->make_null(); }

    TestHandle<Value> convert_arg(bool value) { return ctx_->make_boolean(value); }

    TestHandle<Value> convert_arg(int value) { return convert_arg(i64(value)); }

    TestHandle<Value> convert_arg(i64 value) { return ctx_->make_int(value); }

    TestHandle<Value> convert_arg(f64 value) { return ctx_->make_float(value); }

    TestHandle<Value> convert_arg(const char* value) {
        return convert_arg(std::string_view(value));
    }

    TestHandle<Value> convert_arg(Handle<Value> handle) {
        return TestHandle<Value>(ctx_->ctx(), *handle);
    }

    TestHandle<Value> convert_arg(std::string_view value) { return ctx_->make_string(value); }

private:
    TestContext* ctx_ = nullptr;
    std::string_view function_name_;
    std::vector<TestHandle<Value>> args_;
};

// Test case helpers (contain REQUIRE statements).
void require_null(Handle<Value> handle);
void require_bool(Handle<Value> handle, bool expected);
void require_int(Handle<Value> handle, i64 expected);
void require_float(Handle<Value> handle, f64 expected);
void require_string(Handle<Value> handle, std::string_view expected);

template<typename... Args>
TestCaller TestContext::call(std::string_view function_name, const Args&... args) {
    auto caller = TestCaller(this, function_name);
    caller.args(args...);
    return caller;
}

} // namespace tiro::vm

#endif // TIRO_TEST_SUPPORT_TEST_CONTEXT_HPP
