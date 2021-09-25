#ifndef TIRO_VM_OBJECTS_NATIVE_HPP
#define TIRO_VM_OBJECTS_NATIVE_HPP

#include "common/adt/function_ref.hpp"
#include "common/adt/span.hpp"
#include "common/type_traits.hpp"
#include "vm/handles/external.hpp"
#include "vm/handles/handle.hpp"
#include "vm/handles/span.hpp"
#include "vm/object_support/layout.hpp"
#include "vm/objects/tuple.hpp"
#include "vm/objects/value.hpp"

#include "tiro/objects.h"

#include <functional>
#include <new>

namespace tiro::vm {

enum class NativeFunctionType {
    // TODO: Remove this, currently required because StaticLayout needs default constructible types.
    Invalid,
    Sync,
    Async,
};

std::string_view to_string(NativeFunctionType type);

class NativeFunctionFrame final {
public:
    explicit NativeFunctionFrame(
        Context& ctx, Handle<Coroutine> coro, SyncFrame* frame, OutHandle<Value> return_value);

    NativeFunctionFrame(const NativeFunctionFrame&) = delete;
    NativeFunctionFrame& operator=(const NativeFunctionFrame&) = delete;

    Context& ctx() const { return ctx_; }
    Handle<Coroutine> coro() const;

    Value closure() const;

    size_t arg_count() const;
    Handle<Value> arg(size_t index) const;
    HandleSpan<Value> args() const;

    /// Sets the return slot of this function frame to the value `r`.
    /// The value will be returned to the caller of this function once it returns.
    void return_value(Value r);

    /// Sets the panic slot of this function frame to the value `ex`.
    /// Once the native function returns, the value will be thrown and stack unwinding will take place.
    void panic(Value ex);

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
    Context& ctx_;
    Handle<Coroutine> coro_;
    SyncFrame* frame_ = nullptr;
    OutHandle<Value> return_value_;
};

class NativeAsyncFunctionFrame final {
public:
    explicit NativeAsyncFunctionFrame(Context& ctx, Handle<Coroutine> coro, AsyncFrame* frame);
    NativeAsyncFunctionFrame(NativeAsyncFunctionFrame&&) noexcept;
    ~NativeAsyncFunctionFrame();

    NativeAsyncFunctionFrame(const NativeAsyncFunctionFrame&) = delete;
    NativeAsyncFunctionFrame& operator=(const NativeAsyncFunctionFrame&) = delete;
    NativeAsyncFunctionFrame& operator=(NativeAsyncFunctionFrame&&) = delete;

    Context& ctx() const { return ctx_; }

    Value closure() const;

    size_t arg_count() const;
    Handle<Value> arg(size_t index) const;
    HandleSpan<Value> args() const;
    void return_value(Value v);

    // TODO exceptions!

private:
    // Schedules the coroutine for execution (after setting the return value).
    void resume();

    Handle<Coroutine> coroutine() const;
    AsyncFrame* frame() const;

private:
    Context& ctx_;
    Value* coro_external_ = nullptr; // Reset to null if frame was moved!
    AsyncFrame* frame_ = nullptr;    // Reset to null if already resumed!
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

    /// Returns the type of the native function. Native functions of different types expect
    /// different calling conventions (e.g. sync vs async) and different arguments.
    constexpr NativeFunctionType type() const { return type_; }

    /// Invokes this function as a synchronous function.
    /// \pre `type() == NativeFunctionType::Sync`.
    void invoke_sync(NativeFunctionFrame& frame) {
        TIRO_DEBUG_ASSERT(
            type() == NativeFunctionType::Sync, "Cannot call this function as a sync function.");
        return invoke_sync_(frame, buffer_);
    }

    /// Invokes this function as an asynchronous function.
    /// \pre `type() == NativeFunctionType::Async`.
    void invoke_async(NativeAsyncFunctionFrame frame) {
        TIRO_DEBUG_ASSERT(
            type() == NativeFunctionType::Async, "Cannot call this function as an async function.");
        return invoke_async_(std::move(frame), buffer_);
    }

private:
    template<typename SyncFunction>
    NativeFunctionStorage(ConstructorTag<NativeFunctionType::Sync>, SyncFunction&& func) {
        using Func = remove_cvref_t<SyncFunction>;
        check_function_properties<Func>();

        type_ = NativeFunctionType::Sync;
        invoke_sync_ = invoke_sync_impl<Func>;
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

    template<auto Function>
    constexpr NativeFunctionStorage(ConstexprConstructorTag<NativeFunctionType::Sync, Function>)
        : type_(NativeFunctionType::Sync)
        , invoke_sync_(invoke_static_sync_impl<Function>)
        , buffer_() {}

    template<auto Function>
    constexpr NativeFunctionStorage(ConstexprConstructorTag<NativeFunctionType::Async, Function>)
        : type_(NativeFunctionType::Async)
        , invoke_async_(invoke_static_async_impl<Function>)
        , buffer_() {}

    template<typename Func>
    constexpr void check_function_properties() {
        static_assert(sizeof(Func) <= buffer_size, "Buffer is too small for that function.");
        static_assert(
            alignof(Func) <= alignof(void*), "Buffer is insufficiently aligned for that function.");
        static_assert(
            std::is_trivially_copyable_v<Func>, "The function must be trivial to move around.");
        static_assert(
            std::is_trivially_destructible_v<Func>, "The function must be trivial to destroy.");
    }

    template<typename Func>
    static void invoke_sync_impl(NativeFunctionFrame& frame, void* buffer) {
        Func* func = static_cast<Func*>(buffer);
        (*func)(frame);
    }

    template<typename Func>
    static void invoke_async_impl(NativeAsyncFunctionFrame frame, void* buffer) {
        Func* func = static_cast<Func*>(buffer);
        (*func)(std::move(frame));
    }

    template<auto Func>
    static void invoke_static_sync_impl(NativeFunctionFrame& frame, void*) {
        Func(frame);
    }

    template<auto Func>
    static void invoke_static_async_impl(NativeAsyncFunctionFrame frame, void*) {
        Func(std::move(frame));
    }

private:
    NativeFunctionType type_;
    union {
        void (*invalid_)(); // needed for constexpr default constructor
        void (*invoke_sync_)(NativeFunctionFrame& frame, void* buffer);
        void (*invoke_async_)(NativeAsyncFunctionFrame frame, void* buffer);
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
        NativeFunctionStorage function;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    /// Constructs a new native function from the given arguments.
    static NativeFunction make(Context& ctx, Handle<String> name, MaybeHandle<Value> closure,
        u32 params, const NativeFunctionStorage& function);

    explicit NativeFunction(Value v)
        : HeapValue(v, DebugCheck<NativeFunction>()) {}

    String name();
    Value closure();
    u32 params();

    /// Returns the actual native function.
    NativeFunctionStorage function();

    Layout* layout() const { return access_heap<Layout>(); }
};

template<typename T>
Handle<T> check_instance(NativeFunctionFrame& frame) {
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
