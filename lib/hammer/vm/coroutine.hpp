#ifndef HAMMER_VM_COROUTINE_HPP
#define HAMMER_VM_COROUTINE_HPP

#include "hammer/vm/object.hpp"

namespace hammer::vm {

/**
 * Serves as a call & value stack for a coroutine. Values pushed/popped by instructions
 * are located here, as well as function call frames. The stack's memory is contiguous.
 * 
 * A new stack that is the copy of an old stack (with the same content but with a larger size)
 * can be obtained via CoroutineStack::grow(). Care must be taken with pointers into the old stack
 * (such as existing frame pointes) as they will be different for the new stack.
 */
class CoroutineStack final : public Value {
public:
    struct alignas(Value) Frame {
        Frame* caller = nullptr;  // Points upwards the stack
        FunctionTemplate tmpl;    // Contains executable code etc.
        ClosureContext closure;   // Closure (If any)
        u32 args = 0;             // this many values BEFORE the frame
        u32 locals = 0;           // this many values AFTER the frame
        const byte* pc = nullptr; // Program counter, points into tmpl->code
    };

    // alignment of Frame could be higher than value, then we would have to pad.
    // It cannot be lower.
    static_assert(
        alignof(Frame) == alignof(Value), "Required for stack operations.");

    static_assert(std::is_trivially_destructible_v<Value>);
    static_assert(std::is_trivially_destructible_v<Frame>);
    static_assert(std::is_trivially_copyable_v<Value>);
    static_assert(std::is_trivially_copyable_v<Frame>);

public:
    static CoroutineStack make(Context& ctx, u32 stack_size);

    // new_size must be greater than the old stack size.
    static CoroutineStack
    grow(Context& ctx, Handle<CoroutineStack> old_stack, u32 new_size);

    CoroutineStack() = default;

    explicit CoroutineStack(Value v)
        : Value(v) {
        HAMMER_ASSERT(
            v.is<CoroutineStack>(), "Value is not a coroutine stack.");
    }

    /// Pushes a frame for given function template + closure on the stack.
    /// There must be enough arguments already on the stack to satisfy the function template.
    bool push_frame(FunctionTemplate tmpl, ClosureContext closure);

    /// Returns the top call frame, or null.
    Frame* top_frame();

    /// Removes the top call frame.
    void pop_frame();

    /// The current call frame's arguments.
    Span<Value> args();

    /// The current call frame's local variables.
    Span<Value> locals();

    /// Push a value on the current frame's value stack.
    bool push_value(Value v);

    /// Returns the number of values on the current frame's value stack.
    u32 top_value_count();

    /// Returns a pointer to the topmost value on the current frame's value stack.
    Value* top_value();

    /// Returns a pointer to the n-th topmost value (0 is the the topmost value) on the current
    /// frame's value stack.
    Value* top_value(u32 n);

    /// Returns a span over the topmost n values on the current frames value stack.
    Span<Value> top_values(u32 n);

    /// Removes the topmost value from the current frame's value stack.
    void pop_value();

    /// Removes the n topmost values from the current frame's value stack.
    void pop_values(u32 n);

    u32 stack_used() const noexcept;
    u32 stack_size() const noexcept;
    u32 stack_available() const noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    // Begin and end of the frame's call arguments.
    Value* args_begin(Frame* frame);
    Value* args_end(Frame* frame);

    // Begin and end of the frame's local variables.
    Value* locals_begin(Frame* frame);
    Value* locals_end(Frame* frame);

    // Begin and end of the frame's value stack.
    Value* values_begin(Frame* frame);
    Value* values_end(Frame* frame, byte* max);

    // Number of values on the frame's value stack.
    u32 value_count(Frame* frame, byte* max);

private:
    struct Data;

    inline Data* data() const noexcept;
};

enum class CoroutineState { Ready, Running, Done };

/**
 * A coroutine is a lightweight userland thread. Coroutines are multiplexed
 * over actual operating system threads.
 */
class Coroutine final : public Value {
public:
    static Coroutine
    make(Context& ctx, Handle<String> name, Handle<CoroutineStack> stack);

    Coroutine() = default;

    explicit Coroutine(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Coroutine>(), "Value is not a coroutine.");
    }

    String name() const noexcept;

    CoroutineStack stack() const noexcept;
    void stack(WriteBarrier, Handle<CoroutineStack> stack) noexcept;

    Value result() const noexcept;
    void result(WriteBarrier, Handle<Value> result) noexcept;

    CoroutineState state() const noexcept;
    void state(CoroutineState state) noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

} // namespace hammer::vm

#endif // HAMMER_VM_COROUTINE_HPP
