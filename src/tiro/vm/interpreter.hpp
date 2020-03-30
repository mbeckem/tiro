#ifndef TIRO_VM_INTERPRETER_HPP
#define TIRO_VM_INTERPRETER_HPP

#include "tiro/bytecode/fwd.hpp"
#include "tiro/objects/coroutines.hpp"
#include "tiro/objects/value.hpp"
#include "tiro/vm/fwd.hpp"

#include <array>

namespace tiro::vm {

///The interpreter is responsible for the creation and the execution
/// of coroutines.
class Interpreter {
public:
    Interpreter() = default;

    void init(Context& ctx);

    /// Creates a new coroutine with the given function as its "main" function.
    /// The arguments will be passed to the function once the coroutine starts.
    /// The arguments tuple may be null (for 0 arguments).
    Coroutine make_coroutine(Handle<Value> func, Handle<Tuple> arguments);

    /// Executes the given coroutine until it either completes or yields.
    /// The coroutine must be in a runnable state.
    /// If the coroutine completed, the result can be obtained by calling coro->result().
    void run(Handle<Coroutine> coro);

    template<typename W>
    inline void walk(W&& w);

    Interpreter(const Interpreter&) = delete;
    Interpreter& operator=(const Interpreter&) = delete;

private:
    // The call state is the result of a function call.
    enum class CallResult {
        Continue,  // Continue with execution in another frame
        Evaluated, // Value was evaluated immediately, continue in this frame
        Yield,     // Coroutine must yield because of an async call
    };

    void run_until_block();

    // Called for normal function frames.
    CoroutineState run_frame();

    // Called for async native function frames.
    CoroutineState run_async_frame();

    // Invokes a function object with `argc` arguments. This function implements
    // the Call instruction.
    //
    // State of the stack:
    //      ARG_1 ... ARG_N
    //                    ^ TOP
    [[nodiscard]] CallResult call_function(Handle<Value> function, u32 argc);

    // Invokes either a method or a function attribute on an object (with `argc` arguments, not
    // including the `this` parameter). This function implements the CallMethod instruction
    // and only works together with the LoadMethod instruction.
    //
    // The LoadMethod instruction is responsible for either returning (method_function, object) or
    // (plain_function, null) to the caller. This depends on whether the function to be called
    // is a method or a normal attribute of `object`.
    //
    // Consider the following function call syntax: `object.function(arg1, ..., argn)`.
    // If `function` is a method in object's type, LoadMethod will have returned (function, object). If, on the other hand,
    // `function` is a simple attribute on the object, LoadMethod will have returned (function, null).
    //
    // The state of the stack expected by this function is thus:
    //
    //      OBJECT ARG_1 ... ARG_N         <-- Method call
    //                           ^ TOP
    //
    //      NULL   ARG_1 ... ARG_N         <-- Plain function call
    //                           ^ TOP
    //
    // When call_method runs, it checks the instance parameter (object or null) and passes
    // either `argc` (plain function) or `argc + 1` arguments (method call, `this` becomes the first argument).
    // This technique ensures that a normal (non-method) function will not receive the `this` parameter.
    [[nodiscard]] CallResult call_method(Handle<Value> method, u32 argc);

    // This function is called by both call_function and call_method.
    // It runs the given callee with argc arguments. Depending on the way
    // the function was called, one additional argument may have to be popped
    // from the stack (plain function call with method syntax, see call_method()).
    //
    // argc is the argument count (number of values on the stack that are passed to the function).
    //
    // Warning: Make sure that function is not on the stack (it may grow and therefore invalidate references).
    // Use a register instead!
    [[nodiscard]] CallResult enter_function(
        MutableHandle<Value> function_register, u32 argc, bool pop_one_more);

    // Return from a function call made through enter_function().
    // The current frame is removed and execution should continue in the caller (if any).
    //
    // The given return value will be returned to the calling code. Because this function does not allocate
    // any memory, the naked `Value` passed here is safe.
    //
    // Returns either CoroutineState::Running (continue in current frame) or Done (no more frames). Never yields.
    [[nodiscard]] CoroutineState exit_function(Value return_value);

    // Pushes a value onto the stack. Fails if the stack has no available capacity.
    // Use reseve_values(n) before calling this function.
    //
    // Because this function does not reallocate the stack, pointers into the stack remain valid
    // at all times.
    void must_push_value(Value v);

    // Pushes a new call frame onto the stack.
    // This might cause the underyling stack to grow (which means
    // a relocation of the stack and frame pointer).
    //
    void push_user_frame(
        Handle<FunctionTemplate> tmpl, Handle<Environment> closure, u8 flags);
    void push_async_frame(Handle<NativeAsyncFunction> func, u32 argc, u8 flags);

    // Pops the topmost function call frame.
    void pop_frame();

    // Called by push_frame and pop_frame to sync this instance's attributes
    // with the topmost frame on the stack.
    void update_frame();

    // Make sure that the stack can hold `value_count` additonal values without overflowing.
    // After this function call, `value_count` values can be pushed without an allocation failure.
    // Invalidates pointers into the stack.
    void reserve_values(u32 value_count);

    // Grow the current coroutine's stack. Pointers into the stack must be
    // updated to the new location, so the frame and handles pointing into the stack
    // will be invalidated!
    void grow_stack();

    // Jumps to the given code offset.
    void jump(UserFrame* frame, u32 offset);

    // Allocates a new register slot and returns a handle into it.
    // Registers are reset before every instruction is exected.
    template<typename T = Value>
    MutableHandle<T> reg(T initial) {
        Value* slot = allocate_register_slot();
        *slot = static_cast<Value>(initial);
        return MutableHandle<T>::from_slot(slot);
    }

    // If our registers ever show up in a profiler, we can simply switch to precomputing
    // static indices for every needed register. A bitset (in debug mode) would make sure
    // that there are no conficts between allocated registers.
    Value* allocate_register_slot();

    Context& ctx() const {
        TIRO_ASSERT(ctx_, "Context not initialized.");
        return *ctx_;
    }

private:
    Context* ctx_;

    // The currently executing coroutine.
    Coroutine current_;

    // This is always current_.stack(). This value changes when
    // the stack must be resized.
    CoroutineStack stack_;

    // Points into the stack and is automatically updated when the stack resizes.
    CoroutineFrame* frame_ = nullptr;

    // Temporary values, these are guaranteed to be visited by the GC.
    // Use reg(value) to allocate a register.
    std::array<Value, 16> registers_{};
    byte registers_used_ = 0;
};

template<typename W>
void Interpreter::walk(W&& w) {
    w(current_);
    w(stack_);

    for (byte i = 0; i < registers_used_; ++i) {
        w(registers_[i]);
    }
}

} // namespace tiro::vm

#endif // TIRO_VM_INTERPRETER_HPP
