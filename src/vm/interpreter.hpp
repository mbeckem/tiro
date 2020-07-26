#ifndef TIRO_VM_INTERPRETER_HPP
#define TIRO_VM_INTERPRETER_HPP

#include "compiler/bytecode/fwd.hpp"
#include "vm/fwd.hpp"
#include "vm/handles/handle.hpp"
#include "vm/objects/coroutine.hpp"
#include "vm/objects/value.hpp"

#include <array>

namespace tiro::vm {

/// Storage for temporary values used by the interpreter. Only a limited
/// number of registers are available. The code is written in a way that does not
/// exceed this limit.
class Registers final {
public:
    Registers() = default;

    // Allocates a new register slot and returns a handle into it.
    // Registers are reset (by calling `reset()`) before every instruction is exected.
    template<typename T = Value>
    MutHandle<WrappedType<T>> alloc(const T& initial = T()) {
        Value* slot = alloc_slot();
        *slot = unwrap_value(initial);
        return MutHandle<WrappedType<T>>::from_raw_slot(slot);
    }

    void reset() { slots_used_ = 0; }

    template<typename T>
    void trace(T&& t) {
        for (size_t i = 0, n = slots_used_; i < n; ++i)
            t(slots_[i]);
    }

    Registers(const Registers&) = delete;
    Registers& operator=(const Registers&) = delete;

private:
    // If our registers ever show up in a profiler, we can simply switch to precomputing
    // static indices for every needed register. A bitset (in debug mode) would make sure
    // that there are no conficts between allocated registers.
    Value* alloc_slot();

private:
    // Temporary values, these are guaranteed to be visited by the GC.
    // Use alloc(value) to allocate a register.
    std::array<Value, 16> slots_{};
    byte slots_used_ = 0;
};

/// Handles interpretation of bytecode frames. This class contains the hot loop.
class BytecodeInterpreter final {
public:
    // Constructs a new bytecode interpreter for the given coroutine. The coroutine's topmost frame
    // must be a bytecode frame (UserFrame).
    explicit BytecodeInterpreter(
        Context& ctx, Interpreter& parent, Registers& regs, Coroutine coro, UserFrame* frame);

    // Runs the bytecode of the current function frame and returns on the first suspension point.
    //
    // Possible suspension points are:
    // - A call to another function (including asynchronous ones)
    // - Return from the function
    //
    // In any event, after `run` has finished executing, the topmost frame on the stack will have to be
    // reexamined.
    CoroutineState run();

    template<typename Tracer>
    void trace(Tracer&& tracer) {
        // Note: regs not owned by us, visited in parent
        tracer(coro_);
        tracer(stack_);
    }

private:
    template<typename T>
    auto reg(const T& t) {
        return regs_.alloc(t);
    }

    // Returns from the current function with the given return value.
    // Pops the current frame from the stack and returns the next
    // state of the coroutine.
    CoroutineState exit_function(Value return_value);

    template<typename Func>
    void binop(Func&& fn);

    // Returns the module member with the given index from the current function's module.
    Value get_member(u32 index);

    // Sets the module member with the given index (in the current function's module) to the specified value.
    void set_member(u32 index, Value v);

    // Reserves enough space for `n` additional values on the stack. The stack might grow
    // (and therefore change) as a result.
    void reserve_stack(u32 n);

    // Push the given value on the stack. It is a fatal error if there is not enough space
    // on the stack before calling this function. Use `reserve_stack` when appropriate.
    void push_stack(Value v);

    // Reads a variable of the given type from the instruction stream.
    BytecodeOp read_op();
    i64 read_i64();
    f64 read_f64();
    u32 read_u32();
    MutHandle<Value> read_local();

    // Returns the number of readable bytes in the instruction streams (starting from the current pc).
    size_t readable_bytes() const;

    // Returns true if `offset` is a valid pc value for the current frame.
    bool pc_in_bounds(u32 offset) const;

    // Sets the program counter of the current frame to the given offset. Must be in bounds.
    void set_pc(u32 offset);

private:
    Context& ctx_;
    Interpreter& parent_;
    Registers& regs_;      // Temp storage (TODO: Investigate efficiency?).
    Coroutine coro_;       // Currently executing coroutine.
    CoroutineStack stack_; // The coroutine's stack (changes on growth).
    UserFrame* frame_;     // Current frame (points into the stack, adjusted on stack growth).
};

/// The interpreter is responsible for the creation and the execution
/// of coroutines. The real bytecode interpretation is forwarded to the BytecodeInterpreter class
/// when a call to a bytecode function is made. Native function calls are evaluated directly.
class Interpreter final {
public:
    Interpreter() = default;

    void init(Context& ctx);

    /// Creates a new coroutine with the given function as its "main" function.
    /// The arguments will be passed to the function once the coroutine starts.
    /// The arguments tuple may be null (for 0 arguments).
    Coroutine make_coroutine(Handle<Value> func, MaybeHandle<Tuple> arguments);

    /// Executes the given coroutine until it either completes or yields.
    /// The coroutine must be in a runnable state.
    /// If the coroutine completed, the result can be obtained by calling coro->result().
    void run(Handle<Coroutine> coro);

    template<typename Tracer>
    inline void trace(Tracer&& tracer);

    Interpreter(const Interpreter&) = delete;
    Interpreter& operator=(const Interpreter&) = delete;

private:
    friend BytecodeInterpreter;

    // The call state is the result of a function call.
    enum class CallResult {
        Continue,  // Continue with execution in another frame
        Evaluated, // Value was evaluated immediately, continue in this frame
        Yield,     // Coroutine must yield because of an async call
    };

    void run_until_block(Handle<Coroutine> coro);

    // Initialize the coroutine. This happens when a fresh coroutine ("New" state) is
    // invoked for the first time.
    CoroutineState run_initial(Handle<Coroutine> coro);

    // Run the topmost frame of the coroutine's stack.
    // Note: frame points into the coroutine's current stack and will be invalidated
    // by stack growth during the the interpretation of the function frame.
    CoroutineState run_frame(Handle<Coroutine> coro, UserFrame* frame);
    CoroutineState run_frame(Handle<Coroutine> coro, AsyncFrame* frame);

    // Invokes a function object with `argc` arguments. This function implements
    // the Call instruction.
    //
    // State of the stack:
    //      ARG_1 ... ARG_N
    //                    ^ TOP
    //
    // Note: may invalidate the coroutine's stack because of resizing.
    [[nodiscard]] CallResult
    call_function(Handle<Coroutine> coro, Handle<Value> function, u32 argc);

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
    //
    // Note: may invalidate the coroutine's stack because of resizing.
    [[nodiscard]] CallResult call_method(Handle<Coroutine> coro, Handle<Value> method, u32 argc);

    // This function is called by both call_function and call_method.
    // It runs the given callee with argc arguments. Depending on the way
    // the function was called, one additional argument may have to be popped
    // from the stack (plain function call with method syntax, see call_method()).
    //
    // argc is the argument count (number of values on the stack that are passed to the function).
    //
    // Warning: Make sure that function is not on the stack (it may grow and therefore invalidate references).
    // Use a register instead!
    //
    // Note: may invalidate the coroutine's stack because of resizing.
    [[nodiscard]] CallResult enter_function(
        Handle<Coroutine> coro, MutHandle<Value> function_register, u32 argc, bool pop_one_more);

    // Return from a function call made through enter_function().
    // The current frame is removed and execution should continue in the caller (if any).
    //
    // The given return value will be returned to the calling code. Because this function does not allocate
    // any memory, the raw `Value` passed here is safe.
    //
    // Returns either CoroutineState::Running (continue in current frame) or Done (no more frames). Never yields.
    [[nodiscard]] CoroutineState exit_function(Handle<Coroutine> coro, Value return_value);

    template<typename T>
    auto reg(T&& value) {
        return regs_.alloc(std::forward<T>(value));
    }

    Context& ctx() const {
        TIRO_DEBUG_ASSERT(ctx_, "Context not initialized.");
        return *ctx_;
    }

private:
    Context* ctx_;

    // Enforce that no recursive calls to the interpreter can happen.
    bool running_ = false;

    // Lifeline for the garbage cellector.
    BytecodeInterpreter* child_ = nullptr;

    // Shared temporary storage.
    Registers regs_;

    // Coroutine id counter
    size_t next_id_ = 1;
};

template<typename Tracer>
void Interpreter::trace(Tracer&& tracer) {
    if (child_) {
        child_->trace(tracer);
    }

    regs_.trace(tracer);
}

} // namespace tiro::vm

#endif // TIRO_VM_INTERPRETER_HPP
