#ifndef TIRO_VM_OBJECTS_NATIVE_HPP
#define TIRO_VM_OBJECTS_NATIVE_HPP

#include "common/adt/function_ref.hpp"
#include "common/adt/span.hpp"
#include "common/type_traits.hpp"
#include "vm/handles/external.hpp"
#include "vm/handles/handle.hpp"
#include "vm/handles/span.hpp"
#include "vm/object_support/layout.hpp"
#include "vm/objects/exception.hpp"
#include "vm/objects/tuple.hpp"
#include "vm/objects/value.hpp"

#include "tiro/objects.h"

#include <array>
#include <functional>
#include <new>

namespace tiro::vm {

class ResumableFrameContinuation final {
public:
    enum Action {
        NONE,   // No action
        RETURN, // Return from resumable function
        PANIC,  // Panic from resumable function
        INVOKE, // Invoke another function
        YIELD,  // Put coroutine into waiting state until resumed
    };

    struct RetData {
        Handle<Value> value;
    };

    struct PanicData {
        Handle<Exception> exception;
    };

    struct InvokeData {
        Handle<Function> func;
        Handle<Nullable<Tuple>> args;
    };

    using Registers = std::array<MutHandle<Value>, 2>;

    // The instance needs a few registers as persistent storage
    // to prevent dangling values due to garbage collection.
    // The registers may only be modified from within this instance
    // and must remain valid for as long as the continuation instance is being used.
    ResumableFrameContinuation(const Registers& regs)
        : regs_(regs) {}

    // The continuation action to perform after the native function returned.
    Action action() const { return action_; }

    void do_ret(Value v);
    void do_panic(Exception ex);
    void do_invoke(Function func, Nullable<Tuple> args);
    void do_yield();

    RetData ret_data() const;
    PanicData panic_data() const;
    InvokeData invoke_data() const;

    ResumableFrameContinuation(const ResumableFrameContinuation&) = delete;
    ResumableFrameContinuation& operator=(const ResumableFrameContinuation&) = delete;

private:
    Action action_ = NONE;
    Registers regs_;
};

class ResumableFrameContext final {
public:
    enum WellKnownState {
        START = 0,
        END = -1,
    };

    explicit ResumableFrameContext(Context& ctx, Handle<Coroutine> coro,
        NotNull<ResumableFrame*> frame, ResumableFrameContinuation& cont);

    ResumableFrameContext(ResumableFrameContext&&) = delete;
    ResumableFrameContext& operator=(ResumableFrameContext&&) = delete;

    const ResumableFrameContinuation& cont() const { return cont_; }

    Context& ctx() const { return ctx_; }
    Handle<Coroutine> coro() const;
    Value closure() const;

    size_t arg_count() const;
    Handle<Value> arg(size_t index) const;
    HandleSpan<Value> args() const;

    size_t local_count() const;
    MutHandle<Value> local(size_t index) const;
    MutHandleSpan<Value> locals() const;

    /// Returns the current state of this frame.
    int state() const;

    /// Changes the state of this function frame.
    /// Should be followed by an immediate return from the native function.
    /// It is usually not needed to call this function directly since continuation methods
    /// accept a `next_state` parameter,
    ///
    /// See `ResumeableFrame::WellKnownState` for reserved values.
    void set_state(int new_state);

    /// Indicates that the given function shall be invoked from this frame.
    /// - `func` must refer to a valid function
    /// - `arguments` must be either null or a tuple of arguments appropriate for `func`
    ///
    /// The native function should return immediately without performing any other action on this frame.
    /// It will be resumed with the given state once `func` returned or panicked. (TODO: panic)
    void invoke(int next_state, Function func, Nullable<Tuple> arguments);

    /// Returns the return value of the last function invocation performed by `invoke`.
    /// Should only be used after the function is being resumed after `invoke`.
    ///
    /// TODO: Make panic handling possible
    Value invoke_return();

    /// Retrieves a valid resume token.
    /// Should be used in combination with `yield()`.
    CoroutineToken resume_token();

    /// Pauses the calling coroutine once the native function returns.
    /// The coroutine remains paused until it is being resumed by a valid resume token.
    /// The native function should return immediately without performing any other action on this frame.
    /// Once the coroutine has been resumed, this frame will become active again with the given state.
    void yield(int next_state);

    /// Sets the return slot of this function frame to the value `r`.
    /// The value will be returned to the caller of this function once it returns.
    ///
    /// Note: the state of this frame will be set to `DONE` and it will not be resumed again.
    void return_value(Value r);

    /// Sets the panic slot of this function frame to the value `ex`.
    /// Once the native function returns, the value will be thrown and stack unwinding will take place.
    ///
    /// Note: the state of this frame will be set to `DONE` and it will not be resumed again.
    void panic(Exception ex);

    /// Panics or returns a value, depending on the fallible's state.
    ///
    /// Note: the state of this frame will be set to `DONE`.
    template<typename T>
    void return_or_panic(Fallible<T> fallible) {
        if (fallible.has_exception()) {
            panic(std::move(fallible).exception());
        } else {
            if constexpr (std::is_same_v<T, void>) {
                return_value(Value::null());
            } else {
                return_value(std::move(fallible).value());
            }
        }
    }

private:
    NotNull<ResumableFrame*> frame() const;

private:
    Context& ctx_;
    Handle<Coroutine> coro_;
    ResumableFrame* frame_ = nullptr;
    ResumableFrameContinuation& cont_;
};

class SyncFrameContext final {
public:
    explicit SyncFrameContext(ResumableFrameContext& parent)
        : parent_(parent) {}

    SyncFrameContext(const SyncFrameContext&) = delete;
    SyncFrameContext& operator=(const SyncFrameContext&) = delete;

    Context& ctx() const { return parent_.ctx(); }
    Handle<Coroutine> coro() const { return parent_.coro(); }
    Value closure() const { return parent_.closure(); }

    size_t arg_count() const { return parent_.arg_count(); }
    Handle<Value> arg(size_t index) const { return parent_.arg(index); }
    HandleSpan<Value> args() const { return parent_.args(); }

    /// Sets the return slot of this function frame to the value `r`.
    /// The value will be returned to the caller of this function once it returns.
    void return_value(Value r) { parent_.return_value(r); }

    /// Sets the panic slot of this function frame to the value `ex`.
    /// Once the native function returns, the value will be thrown and stack unwinding will take place.
    void panic(Exception ex) { return parent_.panic(ex); }

    /// Panics or returns a value, depending on the fallible's state.
    template<typename T>
    void return_or_panic(Fallible<T> fallible) {
        parent_.return_or_panic(fallible);
    }

private:
    ResumableFrameContext& parent_;
};

class AsyncFrameContext final {
public:
    enum Local {
        // The local slot used to transport the async return value (if any)
        LOCAL_RESULT = 0,

        // The local slot used to transprot the async panic (if any)
        LOCAL_PANIC,

        LOCALS_COUNT,
    };

    enum State {
        // Start state
        STATE_START = 0,

        // State after resume from yield, return or throw the result
        STATE_RESUME = 1,
    };

    explicit AsyncFrameContext(ResumableFrameContext& parent)
        : parent_(parent) {}

    AsyncFrameContext(const AsyncFrameContext&) = delete;
    AsyncFrameContext& operator=(const AsyncFrameContext&) = delete;

    Context& ctx() const { return parent_.ctx(); }

    Value closure() const { return parent_.closure(); }

    size_t arg_count() const { return parent_.arg_count(); }
    Handle<Value> arg(size_t index) const { return parent_.arg(index); }
    HandleSpan<Value> args() const { return parent_.args(); }

    void return_value(Value r) { parent_.return_value(r); }
    void panic(Exception ex) { parent_.panic(ex); }

    AsyncResumeToken resume_token();
    void yield() { parent_.yield(AsyncFrameContext::STATE_RESUME); }

    template<typename T>
    void return_or_panic(Fallible<T> fallible) {
        parent_.return_or_panic(fallible);
    }

private:
    ResumableFrameContext& parent_;
};

class UnownedAsyncResumeToken final {
public:
    explicit UnownedAsyncResumeToken(External<CoroutineToken> token);

    Context& ctx() const;

    void return_value(Value r);
    void panic(Exception ex);

private:
    void complete(Value v, bool panic);

    Context& get_ctx();

    Coroutine get_coro();

    /// XXX: points to the coroutine stack, do not store the pointer
    /// for long (gc may run or stack may reallocate).
    NotNull<ResumableFrame*> get_frame(Handle<Coroutine> coro);

private:
    External<CoroutineToken> token_;
};

class AsyncResumeToken final {
public:
    explicit AsyncResumeToken(UniqueExternal<CoroutineToken> token);
    ~AsyncResumeToken();

    AsyncResumeToken(AsyncResumeToken&& other) noexcept;
    AsyncResumeToken& operator=(AsyncResumeToken&& other) noexcept;

    Context& ctx() const { return forward().ctx(); }

    void return_value(Value r) { forward().return_value(r); }
    void panic(Exception ex) { forward().panic(ex); }

    External<CoroutineToken> release() { return token_.release(); }

private:
    UnownedAsyncResumeToken forward() const { return UnownedAsyncResumeToken(token_.get()); }

private:
    UniqueExternal<CoroutineToken> token_;
};

class NativeFunctionHolder final {
private:
    static constexpr size_t buffer_size = 2 * sizeof(void*); // Adjust as necessary

    struct DynamicConstructor {};

    template<auto Function>
    struct ConstexprConstructor {};

public:
    template<typename Function>
    static NativeFunctionHolder wrap(Function&& func) {
        return NativeFunctionHolder(DynamicConstructor(), std::forward<Function>(func));
    }

    template<auto Function>
    constexpr static NativeFunctionHolder wrap_static() {
        return NativeFunctionHolder(ConstexprConstructor<Function>());
    }

    /// Constructs an invalid instance.
    /// Needed for default-constructure requirement in current object layout implementation.
    constexpr NativeFunctionHolder()
        : invoke_(nullptr)
        , buffer_() {}

    constexpr NativeFunctionHolder(const NativeFunctionHolder&) = default;
    constexpr NativeFunctionHolder& operator=(const NativeFunctionHolder&) = default;

    /// Returns true if this instance stores a valid function pointer.
    bool valid() const { return invoke_ != nullptr; }

    /// Invokes this function as a resumable function.
    /// \pre `valid()`.
    void invoke(ResumableFrameContext& frame) {
        TIRO_DEBUG_ASSERT(invoke_ != nullptr, "invalid instance");
        return invoke_(frame, buffer_);
    }

private:
    template<typename ResumableFunction>
    NativeFunctionHolder(DynamicConstructor, ResumableFunction&& func) {
        using Func = remove_cvref_t<ResumableFunction>;
        check_function_properties<Func>();
        invoke_ = invoke_impl<Func>;
        new (&buffer_) Func(std::forward<ResumableFunction>(func));
    }

    template<auto Function>
    constexpr NativeFunctionHolder(ConstexprConstructor<Function>)
        : invoke_(invoke_static_impl<Function>)
        , buffer_() {}

    template<typename Func>
    constexpr void check_function_properties() {
        static_assert(sizeof(Func) <= buffer_size, "buffer is too small for that function");
        static_assert(
            alignof(Func) <= alignof(void*), "buffer is insufficiently aligned for that function");
        static_assert(
            std::is_trivially_copyable_v<Func>, "the function must be trivial to move around");
        static_assert(
            std::is_trivially_destructible_v<Func>, "the function must be trivial to destroy");
    }

    template<typename Func>
    static void invoke_impl(ResumableFrameContext& frame, void* buffer) {
        Func* func = static_cast<Func*>(buffer);
        (*func)(frame);
    }

    template<auto Func>
    static void invoke_static_impl(ResumableFrameContext& frame, void*) {
        Func(frame);
    }

private:
    void (*invoke_)(ResumableFrameContext& frame, void* buffer);
    alignas(void*) char buffer_[buffer_size];
};

/// Represents a native function that has been exposed to the calling code.
class NativeFunction final : public HeapValue {
private:
    enum Slots {
        NameSlot,
        ClosureSlot,
        SlotCount_,
    };

    struct Payload {
        u32 params;
        u32 locals;
        NativeFunctionHolder function;
    };

public:
    class Builder final {
    public:
        Builder& params(u32 value) {
            params_ = value;
            return *this;
        }

        Builder& name(Handle<String> value) {
            name_ = value;
            return *this;
        }

        Builder& closure(Handle<Value> value) {
            closure_ = value;
            return *this;
        }

        NativeFunction make(Context& ctx) { return NativeFunction::make_impl(ctx, *this); }

    private:
        friend NativeFunction;

        Builder(const NativeFunctionHolder& holder, u32 locals)
            : holder_(holder)
            , locals_(locals) {}

    private:
        NativeFunctionHolder holder_;
        MaybeHandle<String> name_;
        MaybeHandle<Value> closure_;
        u32 params_ = 0;
        u32 locals_ = 0;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    template<typename SyncFn>
    static Builder sync(SyncFn&& fn) {
        using Adapter = SyncAdapter<remove_cvref_t<SyncFn>>;
        return Builder(NativeFunctionHolder::wrap(Adapter(std::forward<SyncFn>(fn))), 0);
    }

    template<typename AsyncFn>
    static Builder async(AsyncFn&& fn) {
        using Adapter = AsyncAdapter<remove_cvref_t<AsyncFn>>;
        return Builder(NativeFunctionHolder::wrap(Adapter(std::forward<AsyncFn>(fn))),
            AsyncFrameContext::LOCALS_COUNT);
    }

    template<typename ResumableFn>
    static Builder resumable(ResumableFn&& fn, u32 locals = 0) {
        return Builder(NativeFunctionHolder::wrap(std::forward<ResumableFn>(fn)), locals);
    }

    explicit NativeFunction(Value v)
        : HeapValue(v, DebugCheck<NativeFunction>()) {}

    String name();
    Value closure();
    u32 params();
    u32 locals();

    /// Returns the actual native function.
    NativeFunctionHolder function();

    Layout* layout() const { return access_heap<Layout>(); }

private:
    static NativeFunction make_impl(Context& ctx, const Builder& builder);

private:
    template<typename SyncFn>
    struct SyncAdapter {
        SyncFn func;

        explicit SyncAdapter(SyncFn fn)
            : func(std::move(fn)) {}

        void operator()(ResumableFrameContext& frame) {
            TIRO_DEBUG_ASSERT(frame.state() == ResumableFrameContext::START,
                "unexpected frame state when invoking a sync function");

            SyncFrameContext sync(frame);
            func(sync);
            if (frame.cont().action() == ResumableFrameContinuation::NONE)
                frame.return_value(Value::null());
        }
    };

    template<typename AsyncFn>
    struct AsyncAdapter {
        AsyncFn func;

        explicit AsyncAdapter(AsyncFn fn)
            : func(std::move(fn)) {}

        void operator()(ResumableFrameContext& frame) {
            switch (frame.state()) {
            case AsyncFrameContext::STATE_START: {
                AsyncFrameContext async(frame);
                func(async);
                if (frame.cont().action() == ResumableFrameContinuation::NONE)
                    frame.return_value(Value::null());
                return;
            }
            case AsyncFrameContext::STATE_RESUME: {
                auto result = frame.local(AsyncFrameContext::LOCAL_RESULT);
                auto panic = frame.local(AsyncFrameContext::LOCAL_PANIC);
                if (!panic->is_null())
                    return frame.panic(*panic.must_cast<Exception>());
                return frame.return_value(*result);
            }
            }
            TIRO_DEBUG_ASSERT(false, "unexpected frame state when invoking an async function");
        }
    };
};

template<typename T>
Handle<T> check_instance(SyncFrameContext& frame) {
    Handle<Value> value = frame.arg(0);
    if (auto instance = value.try_cast<T>()) {
        return instance.handle();
    }
    TIRO_ERROR("`this` is not a {}", to_string(TypeToTag<T>));
}

class NativeObject final : public HeapValue {
public:
    static constexpr size_t max_alignment = alignof(std::max_align_t);

    struct Payload {
        // TODO: Don't refer to public type directly, introduce an indirection in the API.
        const tiro_native_type_t* type = nullptr;
    };

public:
    /// TODO: Optimize native objects that do not have a finalizer
    using Layout = BufferLayout<byte, max_alignment, StaticPayloadPiece<Payload>, FinalizerPiece>;
    static_assert(LayoutTraits<Layout>::has_finalizer);

    // NOTE: does not check alignment at runtime, only uses asserts
    static NativeObject make(Context& ctx, const tiro_native_type_t* type, size_t size);

    explicit NativeObject(Value v)
        : HeapValue(v, DebugCheck<NativeObject>()) {}

    // The type descriptor of the native object.
    const tiro_native_type_t* native_type();

    // Raw pointer to the native object's user storage.
    void* data();

    // Size of data, in bytes.
    size_t size();

    // Calls the `finalizer` function that was provided during initialization.
    // The garbage collector will always call this function if the object in question is being collected.
    void finalize();

    Layout* layout() const { return access_heap<Layout>(); }
};

/// Wraps a native pointer value. The value is not inspected or owned in any way,
/// the user must make sure that the value remains valid for as long as it is being used.
///
/// Use NativeObject instead if you need more control over the lifetime of native objects.
class NativePointer final : public HeapValue {
private:
    struct Payload {
        void* ptr;
    };

public:
    using Layout = StaticLayout<StaticPayloadPiece<Payload>>;

    static NativePointer make(Context& ctx, void* ptr);

    explicit NativePointer(Value v)
        : HeapValue(v, DebugCheck<NativePointer>()) {}

    void* data() const;

    Layout* layout() const { return access_heap<Layout>(); }
};

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_NATIVE_HPP
