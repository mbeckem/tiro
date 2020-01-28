#ifndef TIRO_TEST_VM_EVAL_CONTEXT_HPP
#define TIRO_TEST_VM_EVAL_CONTEXT_HPP

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
    explicit TestContext(std::string_view source);

    TestHandle<Value> run(std::string_view function_name);

    Context& ctx() {
        TIRO_ASSERT(context_, "Invalid context.");
        return *context_;
    }

private:
    Module compile(std::string_view source);
    Function find_function(Handle<Module> module, std::string_view name);

private:
    std::unique_ptr<Context> context_;
    Global<Module> module_;
};

} // namespace tiro::vm

#endif // TIRO_TEST_VM_EVAL_CONTEXT_HPP
