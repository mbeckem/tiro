#ifndef TIRO_VM_OBJECTS_NATIVE_HPP
#define TIRO_VM_OBJECTS_NATIVE_HPP

#include "common/adt/span.hpp"
#include "common/function_ref.hpp"
#include "common/type_traits.hpp"
#include "vm/handles/external.hpp"
#include "vm/handles/handle.hpp"
#include "vm/handles/span.hpp"
#include "vm/objects/layout.hpp"
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

    void result(Value v);
    // TODO exceptions!

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
    void result(Value v);
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

// C++17 does not allow changing the active member of a union in a constexpr context, so
// the member must be correct right away at the point of construction. This is why we initialize the vtable
// and the class itself awkwardly in its constructor.
struct NativeInvalidTag {};

template<typename Func>
struct NativeSyncTag {};

template<typename Func>
struct NativeAsyncTag {};

struct NativeVTable {
    constexpr NativeVTable(NativeInvalidTag)
        : type(NativeFunctionType::Invalid)
        , invalid(nullptr) {}

    template<typename Func>
    constexpr NativeVTable(NativeSyncTag<Func>)
        : type(NativeFunctionType::Sync)
        , invoke_sync(invoke_sync_impl<Func>) {}

    template<typename Func>
    constexpr NativeVTable(NativeAsyncTag<Func>)
        : type(NativeFunctionType::Async)
        , invoke_async(invoke_async_impl<Func>) {}

    NativeFunctionType type;
    union {
        void (*invalid)();
        void (*invoke_sync)(NativeFunctionFrame& frame, void* buffer);
        void (*invoke_async)(NativeAsyncFunctionFrame frame, void* buffer);
    };

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
};

inline constexpr NativeVTable invalid_vtable{NativeInvalidTag()};

template<typename Func>
inline constexpr NativeVTable sync_vtable{NativeSyncTag<Func>()};

template<typename Func>
inline constexpr NativeVTable async_vtable{NativeAsyncTag<Func>()};

// TODO: This should be constexpr, but it seems to be impossible in C++17 because
// of the internal buffer. Make the type_desc stuff constexpr again if this class becomes constexpr.
// Source: https://stackoverflow.com/a/53087000
class NativeFunctionArg final {
private:
    static constexpr size_t buffer_size = 2 * sizeof(void*); // Adjust as necessary

public:
    constexpr NativeFunctionArg()
        : vtable(&invalid_vtable)
        , buffer() {}

    constexpr NativeFunctionArg(const NativeFunctionArg&) = default;
    constexpr NativeFunctionArg& operator=(const NativeFunctionArg&) = default;

    /// Represents a native function that will be called in a synchronous fashion.
    /// The function should place the function return value in the provided frame.
    /// Only use this approach for simple, nonblocking functions!
    template<typename Function>
    static NativeFunctionArg sync(Function&& func) {
        return NativeFunctionArg(NativeSyncTag<void>(), std::forward<Function>(func));
    }

    /// Similar to `sync()`, but accepts a compile time function pointer (or function like object) to invoke.
    template<auto Function>
    static NativeFunctionArg static_sync() {
        struct Caller {
            void operator()(NativeFunctionFrame& frame) { return Function(frame); }
        };
        return sync(Caller());
    }

    /// Represents a native function that can be called to perform some async operation.
    /// The coroutine will yield and wait until it is resumed by the async operation.
    template<typename Function>
    static NativeFunctionArg async(Function&& func) {
        return NativeFunctionArg(NativeAsyncTag<void>(), std::forward<Function>(func));
    }

    /// Similar to `async()`, but accepts a compile time function pointer (or function like object) to invoke.
    template<auto Function>
    static NativeFunctionArg static_async() {
        struct Caller {
            void operator()(NativeAsyncFunctionFrame frame) { return Function(std::move(frame)); }
        };
        return async(Caller());
    }

    /// Returns the type of the native function. Native functions of different types expect
    /// different calling conventions (e.g. sync vs async) and different arguments.
    constexpr NativeFunctionType type() const {
        TIRO_DEBUG_ASSERT(vtable, "Invalid vtable.");
        return vtable->type;
    }

    /// Invokes this function as a synchronous function.
    /// \pre `type() == NativeFunctionType::Sync`.
    void invoke_sync(NativeFunctionFrame& frame) {
        TIRO_DEBUG_ASSERT(
            type() == NativeFunctionType::Sync, "Cannot call this function as a sync function.");
        return vtable->invoke_sync(frame, buffer);
    }

    /// Invokes this function as an asynchronous function.
    /// \pre `type() == NativeFunctionType::Async`.
    void invoke_async(NativeAsyncFunctionFrame frame) {
        TIRO_DEBUG_ASSERT(
            type() == NativeFunctionType::Async, "Cannot call this function as an async function.");
        return vtable->invoke_async(std::move(frame), buffer);
    }

private:
    template<typename SyncFunction>
    NativeFunctionArg(NativeSyncTag<void>, SyncFunction&& func) {
        using Func = remove_cvref_t<SyncFunction>;
        check_function_properties<Func>();

        vtable = &sync_vtable<Func>;
        new (&buffer) Func(std::forward<SyncFunction>(func));
    }

    template<typename AsyncFunction>
    NativeFunctionArg(NativeAsyncTag<void>, AsyncFunction&& func) {
        using Func = remove_cvref_t<AsyncFunction>;
        check_function_properties<Func>();

        vtable = &async_vtable<Func>;
        new (&buffer) Func(std::forward<AsyncFunction>(func));
    }

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

private:
    const NativeVTable* vtable;
    alignas(void*) char buffer[buffer_size];
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
        NativeFunctionArg function;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    /// Constructs a new native function from the given arguments.
    static NativeFunction make(Context& ctx, Handle<String> name, MaybeHandle<Value> closure,
        u32 params, const NativeFunctionArg& function);

    explicit NativeFunction(Value v)
        : HeapValue(v, DebugCheck<NativeFunction>()) {}

    String name();
    Value closure();
    u32 params();

    /// Returns the actual native function.
    NativeFunctionArg function();

    Layout* layout() const { return access_heap<Layout>(); }
};

template<typename T>
Handle<T> check_instance(NativeFunctionFrame& frame) {
    Handle<Value> value = frame.arg(0);
    if (auto instance = value.try_cast<T>()) {
        return instance.handle();
    }
    TIRO_ERROR("`this` is not a {}.", to_string(TypeToTag<T>));
}

class NativeObject final : public HeapValue {
private:
    struct Payload {
        // TODO: Don't refer to public type directly, introduce an indirection in the API.
        const tiro_native_type_t* type;
    };

public:
    using Layout = BufferLayout<byte, alignof(std::max_align_t), StaticPayloadPiece<Payload>>;

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
