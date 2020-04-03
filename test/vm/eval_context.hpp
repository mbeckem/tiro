#ifndef TIRO_TEST_VM_EVAL_CONTEXT_HPP
#define TIRO_TEST_VM_EVAL_CONTEXT_HPP

#include "tiro/compiler/compiler.hpp"
#include "tiro/heap/handles.hpp"
#include "tiro/vm/context.hpp"

#include <memory>

namespace tiro::vm {

template<typename T>
class TestHandle {
public:
    TestHandle(Context& ctx, T value)
        : handle_(std::make_unique<Global<T>>(ctx, value)) {}

    auto operator-> () { return handle_->operator->(); }

    operator Handle<T>() const { return handle(); }
    Handle<T> handle() const { return handle_->handle(); }

    operator Value() const { return handle_->get(); }

private:
    std::unique_ptr<Global<T>> handle_;
};

class TestContext {
public:
    explicit TestContext(std::string_view source, bool disassemble = false);

    TestHandle<Value> run(std::string_view function_name,
        std::initializer_list<Handle<Value>> arguments = {});

    Context& ctx() {
        TIRO_ASSERT(context_, "Invalid context.");
        return *context_;
    }

    std::string disassemble();

    TestHandle<Value> make_int(i64 value);
    TestHandle<Value> make_string(std::string_view value);
    TestHandle<Value> make_boolean(bool value);

private:
    std::unique_ptr<CompiledModule> compile();
    Function find_function(Handle<Module> module, std::string_view name);

private:
    std::unique_ptr<Context> context_;
    std::unique_ptr<Compiler> compiler_;
    std::unique_ptr<CompiledModule> compiled_;
    Global<Module> module_;
};

} // namespace tiro::vm

#endif // TIRO_TEST_VM_EVAL_CONTEXT_HPP
