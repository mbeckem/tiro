#ifndef HAMMER_VM_OBJECTS_COROUTINE_HPP
#define HAMMER_VM_OBJECTS_COROUTINE_HPP

#include "hammer/vm/objects/function.hpp"
#include "hammer/vm/objects/object.hpp"

namespace hammer::vm {

enum class CoroutineState { Ready, Running, Waiting, Done };

std::string_view to_string(CoroutineState state);

/**
 * Serves as a call & value stack for a coroutine. Values pushed/popped by instructions
 * are located here, as well as function call frames. The stack's memory is contiguous.
 * 
 * A new stack that is the copy of an old stack (with the same content but with a larger size)
 * can be obtained via CoroutineStack::grow(). Care must be taken with pointers into the old stack
 * (such as existing frame pointes) as they will be different for the new stack.
 * 
 * FIXME: Coroutine stacks can move in memory! The stack of the *currently running* coroutine must not be moved.
 */
class CoroutineStack final : public Value {
public:
    static constexpr u32 page_size = 4 << 10;

    // Sizes refer to the object size of the coroutine stack, not the number of
    // available bytes!
    static constexpr u32 initial_size = 4 << 10;
    static constexpr u32 max_size = 16 << 20;

    enum FrameFlags : u8 {
        // Set if we must pop one more value than usual if we return from this function.
        // This is set if a normal function value is called in a a method context, i.e.
        // `a.foo()` where foo is a member value and not a method. There is one more
        // value on the stack (not included in args) that must be cleaned up properly.
        FRAME_POP_ONE_MORE = 1 << 0,
    };

    // Improvement: Call frames could be made more compact.
    // For example, args and locals currently are just copies of their respective values in tmpl.
    // Investigate whether the denormalization is worth it (following the pointer might not be too bad).
    // Also args and locals don't really have to be 32 bit.
    struct alignas(Value) Frame {
        // Points upwards the stack to the caller (or null if this is the topmost frame):
        Frame* caller = nullptr;

        // Contains executable code etc.
        FunctionTemplate tmpl;

        // Context for captured variables (may be null if the function does not have a closure).
        ClosureContext closure;

        // this many values BEFORE the frame on the stack.
        u32 args = 0;

        // this many values AFTER the frame on the stack.
        u32 locals = 0;

        // Bitset of FrameFlags.
        u8 flags = 0;

        // Program counter, points into tmpl->code. FIXME moves
        const byte* pc = nullptr;
    };

    static CoroutineStack make(Context& ctx, u32 object_size);

    // new_size must be greater than the old stack size.
    static CoroutineStack
    grow(Context& ctx, Handle<CoroutineStack> old_stack, u32 new_object_size);

    CoroutineStack() = default;

    explicit CoroutineStack(Value v)
        : Value(v) {
        HAMMER_ASSERT(
            v.is<CoroutineStack>(), "Value is not a coroutine stack.");
    }

    /// Pushes a frame for given function template + closure on the stack.
    /// There must be enough arguments already on the stack to satisfy the function template.
    bool push_frame(FunctionTemplate tmpl, ClosureContext closure, u8 flags);

    /// Returns the top call frame, or null.
    Frame* top_frame();

    /// Removes the top call frame.
    void pop_frame();

    /// Access the function argument at the given index.
    Value* arg(u32 index);
    u32 args_count();

    /// Access the local variable at the given index.
    Value* local(u32 index);
    u32 locals_count();

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

    // alignment of Frame could be higher than value, then we would have to pad.
    // It cannot be lower.
    static_assert(
        alignof(Frame) == alignof(Value), "Required for stack operations.");

    static_assert(std::is_trivially_destructible_v<Value>);
    static_assert(std::is_trivially_destructible_v<Frame>);
    static_assert(std::is_trivially_copyable_v<Value>);
    static_assert(std::is_trivially_copyable_v<Frame>);
};

/**
 * A coroutine is a lightweight userland thread. Coroutines are multiplexed
 * over actual operating system threads.
 */
class Coroutine final : public Value {
public:
    static Coroutine make(Context& ctx, Handle<String> name,
        Handle<Value> function, Handle<CoroutineStack> stack);

    Coroutine() = default;

    explicit Coroutine(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<Coroutine>(), "Value is not a coroutine.");
    }

    String name() const noexcept;
    Value function() const;

    /// The stack of this coroutine. It can be replaced to grow and shrink as needed.
    CoroutineStack stack() const noexcept;
    void stack(Handle<CoroutineStack> stack) noexcept;

    /// The result value of this coroutine (only relevant when the coroutine is done).
    Value result() const noexcept;
    void result(Handle<Value> result) noexcept;

    CoroutineState state() const noexcept;
    void state(CoroutineState state) noexcept;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;
};

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_COROUTINE_HPP
