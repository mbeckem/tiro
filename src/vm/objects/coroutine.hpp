#ifndef TIRO_VM_OBJECTS_COROUTINE_HPP
#define TIRO_VM_OBJECTS_COROUTINE_HPP

#include "vm/objects/function.hpp"
#include "vm/objects/layout.hpp"
#include "vm/objects/native_function.hpp"
#include "vm/objects/string.hpp"

namespace tiro::vm {

enum class CoroutineState { New, Ready, Running, Waiting, Done };

bool is_runnable(CoroutineState state);

std::string_view to_string(CoroutineState state);

enum class FrameType : u8 { User = 0, Async = 1 };

std::string_view to_string(FrameType type);

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
struct alignas(Value) CoroutineFrame {
    // Concrete type of the frame.
    FrameType type;

    // Call flags (bitset of FrameFlags).
    u8 flags = 0;

    // Number of argument values on the stack before this frame.
    u32 args = 0;

    // Number of local variables on the stack after this frame.
    u32 locals = 0;

    // Parent call frame. Null for the first frame on the stack.
    CoroutineFrame* caller = nullptr;

    CoroutineFrame(FrameType type_, u8 flags_, u32 args_, u32 locals_, CoroutineFrame* caller_)
        : type(type_)
        , flags(flags_)
        , args(args_)
        , locals(locals_)
        , caller(caller_) {}
};

/// The UserFrame (TODO BETTER NAME) represents a call to a user defined function.
struct alignas(Value) UserFrame : CoroutineFrame {
    // Contains executable code etc.
    FunctionTemplate tmpl;

    // Context for captured variables (may be null if the function does not have a closure).
    Environment closure;

    // Program counter, points into tmpl->code. FIXME moves
    const byte* pc = nullptr;

    UserFrame(
        u8 flags_, u32 args_, CoroutineFrame* caller_, FunctionTemplate tmpl_, Environment closure_)
        : CoroutineFrame(FrameType::User, flags_, args_, tmpl_.locals(), caller_)
        , tmpl(tmpl_)
        , closure(closure_) {

        TIRO_DEBUG_ASSERT(tmpl_, "Must have a valid function template.");
        TIRO_DEBUG_ASSERT(tmpl_.code(), "Function template must have a code object.");
        // Closure can be null!

        pc = tmpl_.code().data();
    }
};

/// Represents a native function call that can suspend exactly once.
///
/// Coroutine execution is stopped (the state changes to CoroutineState::Waiting) after
/// the async function has been initiated. It is the async function's responsibility
/// to set the return value in this frame and to resume the coroutine (state CoroutineState::Ready).
///
/// The async function may complete immediately. In that case, coroutine resumption is still postponed
/// to the next iteration of the main loop to avoid problems due to unexpect control flow.
struct alignas(Value) AsyncFrame : CoroutineFrame {
    NativeAsyncFunction func;
    Value return_value = Value::null();

    AsyncFrame(u8 flags_, u32 args_, CoroutineFrame* caller_, NativeAsyncFunction func_)
        : CoroutineFrame(FrameType::Async, flags_, args_, 0, caller_)
        , func(func_) {}
};

/// Returns the size (in bytes) of the given coroutine frame. The size depends
/// on the actual type.
size_t frame_size(const CoroutineFrame* frame);

/// Serves as a call & value stack for a coroutine. Values pushed/popped by instructions
/// are located here, as well as function call frames. The stack's memory is contiguous.
///
/// A new stack that is the copy of an old stack (with the same content but with a larger size)
/// can be obtained via CoroutineStack::grow(). Care must be taken with pointers into the old stack
/// (such as existing frame pointes) as they will be different for the new stack.
///
/// The layout of the stack is simple. Call frames and plain values (locals or temporary values) share the same
/// address space within the stack. The call stack grows from the "bottom" to the "top", i.e. the top
/// value (or frame) is the most recently pushed one.
///
/// Example:
///
///  |---------------|
///  |  temp value   |   <- Top of the stack
///  |---------------|
///  |    Local N    |
///  |---------------|
///  |     ....      |
///  |---------------|
///  |    Local 0    |
///  |---------------|
///  |  UserFrame 2  |
///  |---------------|
///  |  ... args ... | <- temporary values
///  |---------------|
///  |  UserFrame 1  | <- Offset 0
///  |---------------|
class CoroutineStack final : public HeapValue {
private:
    struct Payload {
        CoroutineFrame* top_frame = nullptr;
        byte* top = nullptr;
        byte* end = nullptr;
    };

public:
    struct Layout final : Header {
        explicit Layout(Header* type, Value undef_, size_t stack_size)
            : Header(type)
            , undef(undef_) {
            top = &data[0];
            end = &data[stack_size];
            // Unused portions of the stack are uninitialized
        }

        Value undef;
        CoroutineFrame* top_frame = nullptr;
        byte* top;
        byte* end;
        alignas(CoroutineFrame) byte data[];
    };

    // Sizes refer to the object size of the coroutine stack, not the number of
    // available bytes!
    static constexpr u32 initial_size = 1 << 9;
    static constexpr u32 max_size = 1 << 24;

    /// Constructs an empty coroutine stack of the given size.
    /// Called when the interpreter creates a new coroutine - this is the
    /// initial stack.
    static CoroutineStack make(Context& ctx, u32 object_size);

    /// Constructs a new stack as a copy of the old stack.
    /// Uses the given object size as the size for the new stack.
    /// `new_object_size` must be larger than the old_stack's object size.
    ///
    /// The old stack is not modified.
    static CoroutineStack grow(Context& ctx, Handle<CoroutineStack> old_stack, u32 new_object_size);

    CoroutineStack() = default;

    explicit CoroutineStack(Value v)
        : HeapValue(v, DebugCheck<CoroutineStack>()) {}

    /// Pushes a new call frame for given function template + closure on the stack.
    /// There must be enough arguments already on the stack to satisfy the function template.
    bool push_user_frame(FunctionTemplate tmpl, Environment closure, u8 flags);

    /// Pushes a new call frame for the given async function on the stack.
    /// There must be enough arguments on the stack to satisfy the given async function.
    bool push_async_frame(NativeAsyncFunction func, u32 argc, u8 flags);

    /// Returns the top call frame, or null.
    CoroutineFrame* top_frame();

    /// Removes the top call frame.
    void pop_frame();

    /// Access the function argument at the given index.
    Value* arg(u32 index);
    u32 args_count();
    Span<Value> args() { return {args_begin(top_frame()), args_end(top_frame())}; }

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

    /// The number of values that can be pushed without overflowing the current stack's storage.
    u32 value_capacity_remaining();

    /// Used bytes on the stack.
    u32 stack_used();

    /// Total capacity (in bytes) of the stack.
    u32 stack_size();

    /// Bytes on the stack left available.
    u32 stack_available();

    Layout* layout() const { return access_heap<Layout>(); }

private:
    friend LayoutTraits<Layout>;

    template<typename Tracer>
    void trace(Tracer&& t) {
        Layout* data = layout();

        t(data->undef);

        byte* max = data->top;
        CoroutineFrame* frame = data->top_frame;
        while (frame) {
            // Visit all locals and values on the stack; params are not visited here,
            // the upper frame will do it since they are normal values there.
            t(Span<Value>(locals_begin(frame), values_end(frame, max)));

            switch (frame->type) {
            case FrameType::Async: {
                auto async_frame = static_cast<AsyncFrame*>(frame);
                t(async_frame->func);
                t(async_frame->return_value);
                break;
            }
            case FrameType::User: {
                auto user_frame = static_cast<UserFrame*>(frame);
                t(user_frame->tmpl);
                t(user_frame->closure);
                break;
            }
            }

            max = reinterpret_cast<byte*>(frame);
            frame = frame->caller;
        }

        // Values before the first frame
        t(Span<Value>(values_begin(nullptr), values_end(nullptr, max)));
    }

    // Begin and end of the frame's call arguments.
    Value* args_begin(CoroutineFrame* frame);
    Value* args_end(CoroutineFrame* frame);

    // Begin and end of the frame's local variables.
    Value* locals_begin(CoroutineFrame* frame);
    Value* locals_end(CoroutineFrame* frame);

    // Begin and end of the frame's value stack.
    Value* values_begin(CoroutineFrame* frame);
    Value* values_end(CoroutineFrame* frame, byte* max);

    // Number of values on the frame's value stack.
    u32 value_count(CoroutineFrame* frame, byte* max);

    // Allocates a frame by incrementing the top pointer of the stack.
    // Returns nullptr on allocation failure (stack is full).
    //
    // frame_size is the size of the frame structure in bytes.
    // locals is the number of local values to allocate directly after the frame.
    void* allocate_frame(u32 frame_size, u32 locals);

    // Constructs a new stack object with the given dynamic object size (the stack size is
    // slightly lower than that, because of metadata).
    static CoroutineStack make_impl(Context& ctx, u32 object_size);
};

/// A coroutine is a lightweight userland thread. Coroutines are multiplexed
/// over actual operating system threads.
class Coroutine final : public HeapValue {
private:
    struct Payload {
        CoroutineState state = CoroutineState::New;
    };

    enum Slots {
        NameSlot,
        FunctionSlot,
        ArgumentsSlot, // TODO: Nullable
        StackSlot,
        ResultSlot,
        NextReadySlot,
        SlotCount_
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    static Coroutine make(Context& ctx, Handle<String> name, Handle<Value> function,
        Handle<Tuple> arguments, Handle<CoroutineStack> stack);

    Coroutine() = default;

    explicit Coroutine(Value v)
        : HeapValue(v, DebugCheck<Coroutine>()) {
        TIRO_DEBUG_ASSERT(v.is<Coroutine>(), "Value is not a coroutine.");
    }

    String name() const;
    Value function() const;
    Tuple arguments() const; // Optional, can be null

    /// The stack of this coroutine. It can be replaced to grow and shrink as needed.
    CoroutineStack stack() const;
    void stack(Handle<CoroutineStack> stack);

    /// The result value of this coroutine (only relevant when the coroutine is done).
    Value result() const;
    void result(Handle<Value> result);

    CoroutineState state() const;
    void state(CoroutineState state);

    // Linked list of coroutines. Used to implement the set (or queue)
    // of ready coroutines that are waiting for execution.
    Coroutine next_ready() const;
    void next_ready(Coroutine next);

    Layout* layout() const { return access_heap<Layout>(); }
};

template<>
struct LayoutTraits<CoroutineStack::Layout> final {
    using Self = CoroutineStack::Layout;

    static constexpr bool may_contain_references = true;

    static constexpr bool has_static_size = false;

    static size_t dynamic_size(size_t stack_size) { return sizeof(Self) + stack_size; }

    static size_t dynamic_size(Self* instance) {
        return dynamic_size(instance->end - instance->data);
    }

    template<typename Tracer>
    static void trace(Self* instance, Tracer&& t) {
        CoroutineStack(Value::from_heap(instance)).trace(t);
    }
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_COROUTINE_HPP
