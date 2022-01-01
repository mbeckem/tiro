#ifndef TIRO_VM_OBJECTS_COROUTINE_HPP
#define TIRO_VM_OBJECTS_COROUTINE_HPP

#include "common/adt/not_null.hpp"
#include "common/defs.hpp"
#include "vm/fwd.hpp"
#include "vm/object_support/fwd.hpp"
#include "vm/object_support/layout.hpp"
#include "vm/objects/exception.hpp"
#include "vm/objects/function.hpp"
#include "vm/objects/native.hpp"
#include "vm/objects/string.hpp"

namespace tiro::vm {

enum class CoroutineState {
    New,
    Started,
    Ready,
    Running,
    Waiting,
    Done,
};

bool is_runnable(CoroutineState state);

std::string_view to_string(CoroutineState state);

enum class FrameType : u8 { Code = 0, Async = 1, Sync = 2, Catch = 3 };

std::string_view to_string(FrameType type);

enum FrameFlags : u8 {
    // Set if we must pop one more value than usual if we return from this function.
    // This is set if a normal function value is called in a a method context, i.e.
    // `a.foo()` where foo is a field value and not a method. There is one more
    // value on the stack (the unused `this` arg) that must be cleaned up properly.
    FRAME_POP_ONE_MORE = 1 << 0,

    /// Indicates that the function is currently unwinding, i.e. an exception is in flight.
    /// NOTE:
    ///     - code frame: when the bit is set, `current_exception` will contain the in-flight exception value.
    ///     - native frames: signals that the value must be thrown
    ///     - catch frame: exception was caught and stored in `exception`.
    FRAME_UNWINDING = 1 << 1,

    /// Set if the "catch" frame already initiated the wrapped function call.
    FRAME_CATCH_STARTED = 1 << 2,

    // Set if an async function has has it's initialiting function called.
    // This is only valid for frames of type `AsyncFrame`.
    FRAME_ASYNC_CALLED = 1 << 2,

    // Signals that an async function was resumed. This is only valid for frames of type `AsyncFrame`.
    FRAME_ASYNC_RESUMED = 1 << 3,
};

// Improvement: Call frames could be made more compact.
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

    template<typename Tracer>
    void trace(Tracer&&) {}
};

/// The CodeFrame represents a call to a user defined function.
struct alignas(Value) CodeFrame : CoroutineFrame {
    // Contains executable code etc.
    CodeFunctionTemplate tmpl;

    // Context for captured variables (may be null if the function does not have a closure).
    Nullable<Environment> closure;

    // The current exception object. Only useful when the function is unwinding (FRAME_UNWINDING is set).
    // TODO: Can this be stored in the coroutine once instead of wasting a slot per frame?
    Nullable<Exception> current_exception;

    // Program counter, points into tmpl->code. FIXME moves
    const byte* pc = nullptr;

    CodeFrame(u8 flags_, u32 args_, CoroutineFrame* caller_, CodeFunctionTemplate tmpl_,
        Nullable<Environment> closure_)
        : CoroutineFrame(FrameType::Code, flags_, args_, tmpl_.locals(), caller_)
        , tmpl(tmpl_)
        , closure(closure_) {
        pc = tmpl_.code().data();
    }

    template<typename Tracer>
    void trace(Tracer&& t) {
        CoroutineFrame::trace(t);
        t(tmpl);
        t(closure);
        t(current_exception);
    }
};

struct alignas(Value) SyncFrame : CoroutineFrame {
    NativeFunction func;

    SyncFrame(u8 flags_, u32 args_, CoroutineFrame* caller_, NativeFunction func_)
        : CoroutineFrame(FrameType::Sync, flags_, args_, 0, caller_)
        , func(func_) {
        TIRO_DEBUG_ASSERT(func.function().type() == NativeFunctionType::Sync,
            "Unexpected function type (should be sync).");
    }

    template<typename Tracer>
    void trace(Tracer&& t) {
        CoroutineFrame::trace(t);
        t(func);
    }
};

/// Represents a native function call that can suspend exactly once.
///
/// Coroutine execution is stopped (the state changes to CoroutineState::Waiting) after
/// the async function has been initiated. It is the async function's responsibility
/// to set the return value in this frame and to resume the coroutine (state CoroutineState::Ready).
///
/// The async function may complete immediately. In that case, coroutine resumption is still postponed
/// to the next iteration of the main loop to avoid problems due to unexpected control flow.
struct alignas(Value) AsyncFrame : CoroutineFrame {
    NativeFunction func;

    // Either null (function not done yet), the function's return value or an exception (panic).
    // The meaning of this value depends on the frame's flags.
    Value return_value_or_exception = Value::null();

    AsyncFrame(u8 flags_, u32 args_, CoroutineFrame* caller_, NativeFunction func_)
        : CoroutineFrame(FrameType::Async, flags_, args_, 0, caller_)
        , func(func_) {
        TIRO_DEBUG_ASSERT(func.function().type() == NativeFunctionType::Async,
            "Unexpected function type (should be async).");
    }

    template<typename Tracer>
    void trace(Tracer&& t) {
        CoroutineFrame::trace(t);
        t(func);
        t(return_value_or_exception);
    }
};

/// The catch frame is used to implement (primitive) panic handling.
/// It receives a function value as its only argument, which will will then be called when this frame becomes active.
/// Exceptions thrown by the wrapped function will be stored here, i.e. stack unwinding stops at
/// this boundary for non-critical errors.
struct alignas(Value) CatchFrame : CoroutineFrame {
    Nullable<Exception> exception; // set if FRAME_UNWINDING bit is set

    CatchFrame(u8 flags_, u32 args_, CoroutineFrame* caller_)
        : CoroutineFrame(FrameType::Catch, flags_, args_, 0, caller_) {}

    template<typename Tracer>
    void trace(Tracer&& t) {
        CoroutineFrame::trace(t);
        t(exception);
    }
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
///  |  CodeFrame 2  |
///  |---------------|
///  |  ... args ... | <- temporary values
///  |---------------|
///  |  CodeFrame 1  | <- Offset 0
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

    explicit CoroutineStack(Value v)
        : HeapValue(v, DebugCheck<CoroutineStack>()) {}

    /// Pushes a new call frame for given function template + closure on the stack.
    /// There must be enough arguments already on the stack to satisfy the function template.
    bool push_user_frame(CodeFunctionTemplate tmpl, Nullable<Environment> closure, u8 flags);

    /// Pushes a new call frame for the given sync function on the stack.
    /// There must be enough arguments on the stack to satisfy the given function.
    bool push_sync_frame(NativeFunction func, u32 argc, u8 flags);

    /// Pushes a new call frame for the given async function on the stack.
    /// There must be enough arguments on the stack to satisfy the given async function.
    bool push_async_frame(NativeFunction func, u32 argc, u8 flags);

    /// Pushes a new catch frame on the stack.
    bool push_catch_frame(u32 argc, u8 flags);

    /// Returns the top call frame, or null.
    CoroutineFrame* top_frame();

    /// Removes the top call frame.
    void pop_frame();

    /// Access the function argument at the given index.
    static Value* arg(CoroutineFrame* frame, u32 index);
    static Span<Value> args(CoroutineFrame* frame);

    /// Access the local variable at the given index.
    static Value* local(CoroutineFrame* frame, u32 index);

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
            t(Span<Value>(locals_begin(TIRO_NN(frame)), values_end(frame, max)));

            switch (frame->type) {
            case FrameType::Code: {
                static_cast<CodeFrame*>(frame)->trace(t);
                break;
            }
            case FrameType::Sync: {
                static_cast<SyncFrame*>(frame)->trace(t);
                break;
            }
            case FrameType::Async: {
                static_cast<AsyncFrame*>(frame)->trace(t);
                break;
            }
            case FrameType::Catch: {
                static_cast<CatchFrame*>(frame)->trace(t);
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
    static Value* args_begin(NotNull<CoroutineFrame*> frame);
    static Value* args_end(NotNull<CoroutineFrame*> frame);

    // Begin and end of the frame's local variables.
    static Value* locals_begin(NotNull<CoroutineFrame*> frame);
    static Value* locals_end(NotNull<CoroutineFrame*> frame);

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
        ArgumentsSlot,
        StackSlot,
        ResultSlot,
        CurrentTokenSlot,
        NextReadySlot,
        NativeCallbackSlot,
        SlotCount_
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    static Coroutine make(Context& ctx, Handle<String> name, Handle<Value> function,
        MaybeHandle<Tuple> arguments, Handle<CoroutineStack> stack);

    explicit Coroutine(Value v)
        : HeapValue(v, DebugCheck<Coroutine>()) {}

    String name();
    Value function();
    Nullable<Tuple> arguments();

    // The stack of this coroutine. It can be replaced to grow and shrink as needed.
    Nullable<CoroutineStack> stack();
    void stack(Nullable<CoroutineStack> stack);

    // The result value of this coroutine (only relevant when the coroutine is done).
    // When the coroutine is done, then this value must not be null.
    Nullable<Result> result();
    void result(Nullable<Result> result);

    // The current state of the coroutine.
    CoroutineState state();
    void state(CoroutineState state);

    // Native callback that will be executed once this coroutine completes (see coroutine handling
    // in Context class).
    Nullable<NativeObject> native_callback();
    void native_callback(Nullable<NativeObject> callback);

    // Returns the current coroutine token, if any has been created. Tokens are created (and then cached)
    // by calling `create_token`, and they are reset after the coroutine resumes the next time.
    Nullable<CoroutineToken> current_token();

    // Sets the current coroutine token to null. Called when the coroutine is resumed by the Context.
    void reset_token();

    // Linked list of coroutines. Used to implement the set (or queue)
    // of ready coroutines that are waiting for execution.
    Nullable<Coroutine> next_ready();
    void next_ready(Nullable<Coroutine> next);

    // Creates a token suitable to resume this coroutine. The token may only be used once.
    // After this call, the current token will also be resumed from current_token() (which is used
    // to check whether a token is still valid).
    static CoroutineToken create_token(Context& ctx, Handle<Coroutine> coroutine);

    // Yields control to other ready coroutines.
    // The coroutine must be running and will be queued to run after all other currently ready coroutines.
    static void schedule(Context& ctx, Handle<Coroutine> coroutine);

    Layout* layout() const { return access_heap<Layout>(); }
};

template<>
struct LayoutTraits<CoroutineStack::Layout> final {
    using Self = CoroutineStack::Layout;

    static constexpr bool may_contain_references = true;

    static constexpr bool has_finalizer = false;

    static constexpr bool has_static_size = false;

    static size_t dynamic_alloc_size(size_t stack_size) {
        return safe_array_size(sizeof(Self), 1, stack_size);
    }

    static size_t dynamic_size(Self* instance) {
        return unsafe_array_size(sizeof(Self), 1, instance->end - instance->data);
    }

    template<typename Tracer>
    static void trace(Self* instance, Tracer&& t) {
        CoroutineStack(HeapValue(instance)).trace(t);
    }
};

/**
 * A coroutine token allows the user to resume a waiting coroutine. Tokens are invalidated after they
 * have been used, i.e. a coroutine cannot be resumed more than once from the same token.
 */
class CoroutineToken final : public HeapValue {
private:
    enum Slots { CoroutineSlot, SlotCount_ };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    static CoroutineToken make(Context& ctx, Handle<Coroutine> coroutine);

    explicit CoroutineToken(Value v)
        : HeapValue(v, DebugCheck<CoroutineToken>()) {}

    /// Returns the referenced coroutine.
    Coroutine coroutine();

    /// Returns true if this token is still valid, i.e. if it can be used to
    /// resume the referenced coroutine.
    bool valid();

    /// Attempts to resume the referenced coroutine.
    /// Returns true if the coroutine was resumed. In order for this to work the token must be valid
    /// and the coroutine must actually be in the Waiting state.
    static bool resume(Context& ctx, Handle<CoroutineToken> token);

    Layout* layout() const { return access_heap<Layout>(); }
};

extern const TypeDesc coroutine_type_desc;
extern const TypeDesc coroutine_token_type_desc;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_COROUTINE_HPP
