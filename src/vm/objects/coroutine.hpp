#ifndef TIRO_VM_OBJECTS_COROUTINE_HPP
#define TIRO_VM_OBJECTS_COROUTINE_HPP

#include "common/adt/not_null.hpp"
#include "common/defs.hpp"
#include "vm/fwd.hpp"
#include "vm/object_support/fwd.hpp"
#include "vm/objects/coroutine_stack.hpp"

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
    // After this call, the new token will also be returned from current_token() (which is used
    // to check whether a token is still valid).
    static CoroutineToken create_token(Context& ctx, Handle<Coroutine> coroutine);

    // Yields control to other ready coroutines.
    // The coroutine must be running and will be queued to run after all other currently ready coroutines.
    static void schedule(Context& ctx, Handle<Coroutine> coroutine);

    Layout* layout() const { return access_heap<Layout>(); }
};

/// A coroutine token allows the user to resume a waiting coroutine. Tokens are invalidated after they
/// have been used, i.e. a coroutine cannot be resumed more than once from the same token.
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
