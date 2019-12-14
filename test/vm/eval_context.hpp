#ifndef HAMMER_TEST_VM_EVAL_CONTEXT_HPP
#define HAMMER_TEST_VM_EVAL_CONTEXT_HPP

#include "hammer/vm/context.hpp"
#include "hammer/vm/heap/handles.hpp"

#include <memory>

namespace hammer::vm {

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
    TestContext();

    TestHandle<Value>
    compile_and_run(std::string_view view, std::string_view function_name);

    Context& ctx() {
        HAMMER_ASSERT(context_, "Invalid context.");
        return *context_;
    }

private:
    Module compile(std::string_view source);
    Function find_function(Handle<Module> module, std::string_view name);

private:
    std::unique_ptr<Context> context_;
};

} // namespace hammer::vm

#endif // HAMMER_TEST_VM_EVAL_CONTEXT_HPP
