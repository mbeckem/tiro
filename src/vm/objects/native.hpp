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

#include <functional>
#include <new>

namespace tiro::vm {

enum class NativeFunctionType {
    Invalid,
    Sync,
    Async,
    Resumable,
};

std::string_view to_string(NativeFunctionType type);

class AsyncFrameContext final {
public:
    explicit AsyncFrameContext(Context& ctx, Handle<Coroutine> coro, NotNull<AsyncFrame*> frame);
    AsyncFrameContext(AsyncFrameContext&&) noexcept;
    ~AsyncFrameContext();

    AsyncFrameContext(const AsyncFrameContext&) = delete;
    AsyncFrameContext& operator=(const AsyncFrameContext&) = delete;
    AsyncFrameContext& operator=(AsyncFrameContext&&) = delete;

    Context& ctx() const { return ctx_; }

    Value closure() const;

    size_t arg_count() const;
    Handle<Value> arg(size_t index) const;
    HandleSpan<Value> args() const;

    /// Sets the return slot of this function frame to the value `r`.
    /// The value will be returned to the caller of this function once it returns.
    void return_value(Value r);

    /// Sets the panic slot of this function frame to the value `ex`.
    /// Once the native function returns, the value will be thrown and stack unwinding will take place.
    void panic(Exception ex);

    /// Panics or returns a value, depending on the fallible's state.
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
    // Schedules the coroutine for execution (after setting the return value).
    void resume();

    Handle<Coroutine> coroutine() const;
    NotNull<AsyncFrame*> frame() const;

private:
    Context& ctx_;
    Value* coro_external_ = nullptr; // Reset to null if frame was moved!
    AsyncFrame* frame_ = nullptr;    // Reset to null if already resumed!
};

struct ResumableFrameContinuation {
    enum Action {
        NONE,   // No action
        RETURN, // Final return from resumable function
        PANIC,  // Panic from resumable function
        INVOKE, // Invoke another function
    };

    // The continuation action to perform after the native function returned.
    Action action = NONE;

    // Action
    // - None: ignored
    // - Return: return value
    // - Panic: exception
    // - Invoke: function to invoke
    MutHandle<Value> value;

    // Only if action is invoke: function arguments
    MutHandle<Nullable<Tuple>> invoke_arguments;

    ResumableFrameContinuation(const ResumableFrameContinuation&) = delete;
    ResumableFrameContinuation& operator=(const ResumableFrameContinuation&) = delete;
};

class ResumableFrameContext final {
public:
    enum WellKnownState {
        START = 0,
        END = -1,
        CLEANUP = -2,
    };

    explicit ResumableFrameContext(Context& ctx, Handle<Coroutine> coro,
        NotNull<ResumableFrame*> frame, ResumableFrameContinuation& cont);

    ResumableFrameContext(ResumableFrameContext&&) = delete;
    ResumableFrameContext& operator=(ResumableFrameContext&&) = delete;

    Context& ctx() const { return ctx_; }
    Handle<Coroutine> coro() const;
    Value closure() const;

    size_t arg_count() const;
    Handle<Value> arg(size_t index) const;
    HandleSpan<Value> args() const;

    size_t local_count() const;
    Handle<Value> local(size_t index) const;
    HandleSpan<Value> locals() const;

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
    /// It will be resumed with the configured state once `func` returned or panicked. (TODO: panic)
    void invoke(int next_state, Value func, Nullable<Tuple> arguments);

    /// Returns the return value of the last function invocation performed by `invoke`.
    /// Should only be used after the function is being resumed after `invoke`.
    ///
    /// TODO: Make panic handling possible
    Value invoke_return();

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

class NativeFunctionStorage final {
private:
    static constexpr size_t buffer_size = 2 * sizeof(void*); // Adjust as necessary

    template<NativeFunctionType type>
    struct ConstructorTag {};

    template<NativeFunctionType type, auto Function>
    struct ConstexprConstructorTag {};

public:
    constexpr NativeFunctionStorage()
        : type_(NativeFunctionType::Invalid)
        , invalid_()
        , buffer_() {}

    constexpr NativeFunctionStorage(const NativeFunctionStorage&) = default;
    constexpr NativeFunctionStorage& operator=(const NativeFunctionStorage&) = default;

    /// Represents a native function that will be called in a synchronous fashion.
    /// The function should place the function return value in the provided frame.
    /// Only use this approach for simple, nonblocking functions!
    template<typename Function>
    static NativeFunctionStorage sync(Function&& func) {
        return NativeFunctionStorage(
            ConstructorTag<NativeFunctionType::Sync>(), std::forward<Function>(func));
    }

    /// Similar to `sync()`, but accepts a compile time function pointer (or function like object) to invoke.
    template<auto Function>
    constexpr static NativeFunctionStorage static_sync() {
        return NativeFunctionStorage(ConstexprConstructorTag<NativeFunctionType::Sync, Function>());
    }

    /// Represents a native function that can be called to perform some async operation.
    /// The coroutine will yield and wait until it is resumed by the async operation.
    template<typename Function>
    static NativeFunctionStorage async(Function&& func) {
        return NativeFunctionStorage(
            ConstructorTag<NativeFunctionType::Async>(), std::forward<Function>(func));
    }

    /// Similar to `async()`, but accepts a compile time function pointer (or function like object) to invoke.
    template<auto Function>
    constexpr static NativeFunctionStorage static_async() {
        return NativeFunctionStorage(
            ConstexprConstructorTag<NativeFunctionType::Async, Function>());
    }

    /// Represents a native function that can be called as a resumable function.
    /// The coroutine can yield and resume any number of times.
    template<typename Function>
    static NativeFunctionStorage resumable(Function&& func) {
        return NativeFunctionStorage(
            ConstructorTag<NativeFunctionType::Resumable>(), std::forward<Function>(func));
    }

    /// Similar to `resumable()`, but accepts a compile time function pointer (or function like object) to invoke.
    template<auto Function>
    constexpr static NativeFunctionStorage static_resumable() {
        return NativeFunctionStorage(
            ConstexprConstructorTag<NativeFunctionType::Resumable, Function>());
    }

    /// Returns the type of the native function. Native functions of different types expect
    /// different calling conventions (e.g. sync vs async) and different arguments.
    constexpr NativeFunctionType type() const { return type_; }

    /// Invokes this function as an asynchronous function.
    /// \pre `type() == NativeFunctionType::Async`.
    void invoke_async(AsyncFrameContext frame) {
        TIRO_DEBUG_ASSERT(
            type() == NativeFunctionType::Async, "cannot call this function as an async function");
        return invoke_async_(std::move(frame), buffer_);
    }

    /// Invokes this function as a resumable function.
    /// \pre `type() == NativeFunctionType::Resumable`.
    void invoke_resumable(ResumableFrameContext& frame) {
        TIRO_DEBUG_ASSERT(type() == NativeFunctionType::Resumable,
            "cannot call this function as a resumable function");

        return invoke_resumable_(frame, buffer_);
    }

private:
    template<typename SyncFunction>
    NativeFunctionStorage(ConstructorTag<NativeFunctionType::Sync>, SyncFunction&& func) {
        using Func = remove_cvref_t<SyncFunction>;
        check_function_properties<Func>();

        type_ = NativeFunctionType::Resumable;
        invoke_resumable_ = invoke_sync_impl<Func>;
        new (&buffer_) Func(std::forward<SyncFunction>(func));
    }

    template<typename AsyncFunction>
    NativeFunctionStorage(ConstructorTag<NativeFunctionType::Async>, AsyncFunction&& func) {
        using Func = remove_cvref_t<AsyncFunction>;
        check_function_properties<Func>();

        type_ = NativeFunctionType::Async;
        invoke_async_ = invoke_async_impl<Func>;
        new (&buffer_) Func(std::forward<AsyncFunction>(func));
    }

    template<typename ResumableFunction>
    NativeFunctionStorage(ConstructorTag<NativeFunctionType::Resumable>, ResumableFunction&& func) {
        using Func = remove_cvref_t<ResumableFunction>;
        check_function_properties<Func>();

        type_ = NativeFunctionType::Resumable;
        invoke_resumable_ = invoke_resumable_impl<Func>;
        new (&buffer_) Func(std::forward<ResumableFunction>(func));
    }

    template<auto Function>
    constexpr NativeFunctionStorage(ConstexprConstructorTag<NativeFunctionType::Sync, Function>)
        : type_(NativeFunctionType::Resumable)
        , invoke_resumable_(invoke_static_sync_impl<Function>)
        , buffer_() {}

    template<auto Function>
    constexpr NativeFunctionStorage(ConstexprConstructorTag<NativeFunctionType::Async, Function>)
        : type_(NativeFunctionType::Async)
        , invoke_async_(invoke_static_async_impl<Function>)
        , buffer_() {}

    template<auto Function>
    constexpr NativeFunctionStorage(
        ConstexprConstructorTag<NativeFunctionType::Resumable, Function>)
        : type_(NativeFunctionType::Resumable)
        , invoke_resumable_(invoke_static_resumable_impl<Function>)
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
    static void invoke_sync_impl(ResumableFrameContext& frame, void* buffer) {
        Func* func = static_cast<Func*>(buffer);
        translate_to_sync(frame, [&](SyncFrameContext& sync) { (*func)(sync); });
    }

    template<typename Func>
    static void invoke_async_impl(AsyncFrameContext frame, void* buffer) {
        Func* func = static_cast<Func*>(buffer);
        (*func)(std::move(frame));
    }

    template<typename Func>
    static void invoke_resumable_impl(ResumableFrameContext& frame, void* buffer) {
        Func* func = static_cast<Func*>(buffer);
        (*func)(frame);
    }

    template<auto Func>
    static void invoke_static_sync_impl(ResumableFrameContext& frame, void*) {
        translate_to_sync(frame, [](SyncFrameContext& sync) { Func(sync); });
    }

    template<auto Func>
    static void invoke_static_async_impl(AsyncFrameContext frame, void*) {
        Func(std::move(frame));
    }

    template<auto Func>
    static void invoke_static_resumable_impl(ResumableFrameContext& frame, void*) {
        Func(frame);
    }

    template<typename SyncFunc>
    static void translate_to_sync(ResumableFrameContext& frame, SyncFunc&& func) {
        // TODO: Opt out of cleanup via flags to get rid of cleanup state handling?
        TIRO_DEBUG_ASSERT(frame.state() == ResumableFrameContext::START
                              || frame.state() == ResumableFrameContext::CLEANUP,
            "unexpected frame state when invoking a sync function");

        if (frame.state() != ResumableFrameContext::START)
            return; // Prevent cleanup call

        SyncFrameContext sync(frame);
        func(sync);
        if (frame.state() != ResumableFrameContext::END)
            frame.return_value(Value::null());
    }

private:
    NativeFunctionType type_;
    union {
        void (*invalid_)(); // needed for constexpr default constructor
        void (*invoke_async_)(AsyncFrameContext frame, void* buffer);
        void (*invoke_resumable_)(ResumableFrameContext& frame, void* buffer);
    };
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
        NativeFunctionStorage function;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    /// Constructs a new native function from the given arguments.
    static NativeFunction make(Context& ctx, Handle<String> name, MaybeHandle<Value> closure,
        u32 params, u32 locals, const NativeFunctionStorage& function);

    explicit NativeFunction(Value v)
        : HeapValue(v, DebugCheck<NativeFunction>()) {}

    String name();
    Value closure();
    u32 params();
    u32 locals();

    /// Returns the actual native function.
    NativeFunctionStorage function();

    Layout* layout() const { return access_heap<Layout>(); }
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
private:
    struct Payload {
        // TODO: Don't refer to public type directly, introduce an indirection in the API.
        const tiro_native_type_t* type;
    };

public:
    using Layout =
        BufferLayout<byte, alignof(std::max_align_t), StaticPayloadPiece<Payload>, FinalizerPiece>;
    static_assert(LayoutTraits<Layout>::has_finalizer);

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
