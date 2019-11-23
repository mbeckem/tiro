#ifndef HAMMER_VM_INTERPRETER_HPP
#define HAMMER_VM_INTERPRETER_HPP

#include "hammer/vm/fwd.hpp"
#include "hammer/vm/objects/coroutine.hpp"
#include "hammer/vm/objects/value.hpp"

#include <array>

namespace hammer::vm {

/**
 * The interpreter is responsible for the creation and the execution
 * of coroutines.
 */
class Interpreter {
public:
    Interpreter() = default;

    void init(Context& ctx);

    template<typename W>
    void walk(W&& w) {
        w(current_);
        w(stack_);

        for (byte i = 0; i < registers_used_; ++i) {
            w(registers_[i]);
        }
    }

    Coroutine create_coroutine(Handle<Function> fn);
    Value run(Handle<Coroutine> coro);

    Interpreter(const Interpreter&) = delete;
    Interpreter& operator=(const Interpreter&) = delete;

private:
    Value run_until_complete(Handle<Coroutine> coro);
    void run_frame(Handle<Coroutine> coro);

    /// Allocates a new register slot and returns a handle into it.
    /// Registers are reset before every instruction is exected.
    template<typename T = Value>
    MutableHandle<T> reg(T initial) {
        Value* slot = allocate_register_slot();
        *slot = static_cast<Value>(initial);
        return MutableHandle<T>::from_slot(slot);
    }

    Value* allocate_register_slot();

    /// Pushes a value onto the stack.
    /// This might cause the underlying stack to grow (which means
    /// a relocation of the stack and frame pointer).
    ///
    /// Note: it is fine if this value is not rooted, it will be rooted
    /// if a reallocation is necessary in the slow path of the function.
    void push_value(Value v);

    /// Pushes a new call frame onto the stack.
    /// This might cause the underyling stack to grow (which means
    /// a relocation of the stack and frame pointer).
    void
    push_frame(Handle<FunctionTemplate> tmpl, Handle<ClosureContext> closure);

    /// Grow the current coroutine's stack. Pointers into the stack must be
    /// updated to the new location, so the frame and handles pointing into the stack
    /// will be invalidated!
    void grow_stack();

    Context& ctx() const {
        HAMMER_ASSERT(ctx_, "Context not initialized.");
        return *ctx_;
    }

private:
    Context* ctx_;
    Coroutine current_;
    CoroutineStack stack_;
    CoroutineStack::Frame* frame_ = nullptr;

    std::array<Value, 16> registers_{};
    byte registers_used_ = 0;
};

} // namespace hammer::vm

#endif // HAMMER_VM_INTERPRETER_HPP
