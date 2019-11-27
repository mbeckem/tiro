#ifndef HAMMER_VM_INTERPRETER_HPP
#define HAMMER_VM_INTERPRETER_HPP

#include "hammer/compiler/opcodes.hpp"
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

    /// Creates a new coroutine with the given function as its "main" function.
    Coroutine create_coroutine(Handle<Function> fn);

    /// Executes the given coroutine and returns its result.
    /// TODO: need async api
    Value run(Handle<Coroutine> coro);

    template<typename W>
    inline void walk(W&& w);

    Interpreter(const Interpreter&) = delete;
    Interpreter& operator=(const Interpreter&) = delete;

private:
    enum class CallState {
        Continue,  // Continue with bytecode execution in another frame
        Evaluated, // Value was evaluated immediately
        Yield,     // Coroutine must yield because of an async call
    };

private:
    void run_instructions();

    /* 
     * Invokes a function object with `argc` arguments. This function implements
     * the CALL instruction.
     * 
     * State of the stack:
     *      FUNCTION ARG_1 ... ARG_N 
     *                         ^ TOP
     */
    [[nodiscard]] CallState call_function(u32 argc);

    /* 
     * Invokes either a method or a function attribute on an object (with `argc` arguments, not
     * including the `this` parameter). This function implements the CALL_METHOD instruction
     * and only works together with the LOAD_METHOD instruction.
     * 
     * The LOAD_METHOD instruction is responsible for either putting (method_function, object) or
     * (plain_function, null) on the stack. This depends on whether the function to be called
     * is a method or a normal attribute of `object`.
     * 
     * Consider the following function call syntax: `object.function(arg1, ..., argn)`.
     * If `function` is a method in object's type, LOAD_METHOD will have pushed (function, object). If, on the other hand,
     * `function` is a simple attribute on the object, LOAD_METHOD will have pushed (function, null).
     * 
     * The state of the stack is thsus:
     * 
     *      FUNCTION OBJECT ARG_1 ... ARG_N         <-- Method call
     *                                ^ TOP
     * 
     *      FUNCTION NULL   ARG_1 ... ARG_N         <-- Plain function call
     *                                ^ TOP
     * 
     * When call_method runs, it checks the instance parameter (object or null) and passes
     * either `argc` (plain function) or `argc + 1` arguments (method call, `this` becomes the first argument).
     * This technique ensures that a normal (non-method) function will not receive the `this` parameter.
     */
    [[nodiscard]] CallState call_method(u32 argc);

    /*
     * This function is called by both call_function and call_method.
     * It runs the given callee with argc arguments. Depending on the way
     * the function was called, one additional argument may have to be popped
     * from the stack (plain function call with method syntax, see call_method()).
     */
    [[nodiscard]] CallState do_function_call(
        MutableHandle<Value> function, u32 argc, bool pop_one_more);

    // Pushes a value onto the stack.
    // This might cause the underlying stack to grow (which means
    // a relocation of the stack and frame pointer).
    //
    // Note: it is fine if this value is not rooted, it will be rooted
    // if a reallocation is necessary in the slow path of the function.
    void push_value(Value v);

    // Pushes a new call frame onto the stack.
    // This might cause the underyling stack to grow (which means
    // a relocation of the stack and frame pointer).
    //
    void push_frame(Handle<FunctionTemplate> tmpl,
        Handle<ClosureContext> closure, u8 flags);

    // Pops the topmost function call frame.
    void pop_frame();

    // Called by push_frame and pop_frame to sync this instance's attributes
    // with the topmost frame on the stack.
    void update_frame();

    // Grow the current coroutine's stack. Pointers into the stack must be
    // updated to the new location, so the frame and handles pointing into the stack
    // will be invalidated!
    void grow_stack();

    // These function read instructions and raw values from the instruction stream
    // at the current program counter.
    Opcode read_op();
    i64 read_i64();
    f64 read_f64();
    u32 read_u32();

    // Number of readable bytes, starting with the current program counter.
    size_t readable() const;

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
        HAMMER_ASSERT(ctx_, "Context not initialized.");
        return *ctx_;
    }

private:
    Context* ctx_;
    Coroutine current_;
    CoroutineStack stack_;
    CoroutineStack::Frame* frame_ = nullptr; // Points into the stack
    FunctionTemplate tmpl_;                  // Current function template
    Span<const byte> code_;                  // tmpl_->code

    std::array<Value, 16> registers_{};
    byte registers_used_ = 0;
};

template<typename W>
void Interpreter::walk(W&& w) {
    w(current_);
    w(stack_);
    w(tmpl_);

    for (byte i = 0; i < registers_used_; ++i) {
        w(registers_[i]);
    }
}

} // namespace hammer::vm

#endif // HAMMER_VM_INTERPRETER_HPP
