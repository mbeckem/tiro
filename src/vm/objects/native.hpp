#ifndef TIRO_VM_OBJECTS_NATIVE_HPP
#define TIRO_VM_OBJECTS_NATIVE_HPP

#include "common/function_ref.hpp"
#include "common/span.hpp"
#include "vm/handles/global.hpp"
#include "vm/handles/handle.hpp"
#include "vm/handles/span.hpp"
#include "vm/objects/layout.hpp"
#include "vm/objects/tuple.hpp"
#include "vm/objects/value.hpp"

#include <functional>

namespace tiro::vm {

enum class NativeFunctionType {
    Sync,
    Async,
};

std::string_view to_string(NativeFunctionType type);

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
        NativeFunctionType function_type;
        union {
            NativeFunctionPtr sync_function;
            NativeAsyncFunctionPtr async_function;
        };
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    /// Represents a native function that returns immediately. These functions should not block
    /// to avoid halting the main loop (use async functions instead).
    static NativeFunction make(Context& ctx, Handle<String> name, MaybeHandle<Value> closure,
        u32 params, NativeFunctionPtr function);

    /// Represents a native function that can be called to perform some async operation.
    /// The coroutine will yield and wait until it is resumed by the async operation.
    static NativeFunction make(Context& ctx, Handle<String> name, MaybeHandle<Value> closure,
        u32 params, NativeAsyncFunctionPtr function);

    explicit NativeFunction(Value v)
        : HeapValue(v, DebugCheck<NativeFunction>()) {}

    String name();
    Value closure();
    u32 params();

    // Determines the type of the native function.
    NativeFunctionType function_type();

    // function_type() must be Sync.
    NativeFunctionPtr sync_function();

    // function_type() must be Async.
    NativeAsyncFunctionPtr async_function();

    Layout* layout() const { return access_heap<Layout>(); }
};

class NativeFunctionFrame final {
public:
    Context& ctx() const { return ctx_; }
    Handle<Coroutine> coro() const;

    Value closure() const;

    size_t arg_count() const;
    Handle<Value> arg(size_t index) const;
    HandleSpan<Value> args() const { return args_; }

    void result(Value v);
    // TODO exceptions!

    NativeFunctionFrame(const NativeFunctionFrame&) = delete;
    NativeFunctionFrame& operator=(const NativeFunctionFrame&) = delete;

    explicit NativeFunctionFrame(Context& ctx, Handle<Coroutine> coro,
        Handle<NativeFunction> function, HandleSpan<Value> args, MutHandle<Value> result);

private:
    Context& ctx_;
    Handle<Coroutine> coro_;
    Handle<NativeFunction> function_;
    HandleSpan<Value> args_;
    MutHandle<Value> result_;
};

class NativeAsyncFunctionFrame final {
public:
    Context& ctx() const { return storage().coro_.ctx(); }

    Value closure() const;

    size_t arg_count() const;
    Value arg(size_t index) const;
    void result(Value v);
    // TODO exceptions!

    NativeAsyncFunctionFrame(const NativeAsyncFunctionFrame&) = delete;
    NativeAsyncFunctionFrame& operator=(const NativeAsyncFunctionFrame&) = delete;

    NativeAsyncFunctionFrame(NativeAsyncFunctionFrame&&) noexcept = default;
    NativeAsyncFunctionFrame& operator=(NativeAsyncFunctionFrame&&) noexcept = default;

    explicit NativeAsyncFunctionFrame(Context& ctx, Handle<Coroutine> coro);

    ~NativeAsyncFunctionFrame();

private:
    struct Storage {
        Global<Coroutine> coro_;

        Storage(Context& ctx, Handle<Coroutine> coro);
    };

    Storage& storage() const {
        TIRO_DEBUG_ASSERT(storage_, "Invalid frame object (either moved or already resumed).");
        return *storage_;
    }

    // Schedules the coroutine for execution (after setting the return value).
    void resume();

    CoroutineStack stack() const;
    AsyncFrame* frame() const;

private:
    // TODO allocator
    std::unique_ptr<Storage> storage_;
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
public:
    using FinalizerFn = void (*)(void* data, size_t size);

private:
    struct Payload {
        // Unique address to discriminate native types. Nullpointer -> not initialized.
        const void* tag = nullptr;

        // The finalizer is optional, even for initialized objects.
        FinalizerFn finalizer = nullptr;
    };

public:
    using Layout = BufferLayout<byte, alignof(std::max_align_t), StaticPayloadPiece<Payload>>;

    static NativeObject make(Context& ctx, size_t size);

    explicit NativeObject(Value v)
        : HeapValue(v, DebugCheck<NativeObject>()) {}

    // Initializes the native object. Will first call the `constructor` function by passing
    // it the address of the uninitialized storage (and its size). After construction
    // is successful, the tag and the finalize member will be initialized as well.
    //
    // The `tag` value must be a unique value that identifies the native object's inner type to the caller. It must not be null.
    //
    // The `finalizer` function will be called exectly once and will receive the the same (pointer, size) as the `constructor` function. Its
    // responsibility is to cleanup all allocated memory that may be held by the native object.
    void
    construct(const void* tag, FunctionRef<void(void*, size_t)> constructor, FinalizerFn finalizer);

    // Calls the `finalizer` function that was provided during initialization.
    // The function may be called multiple times and will do nothing if the object has already been finalized.
    // The garbage collector will always call this function if the object in question is being collected.
    void finalize();

    // The unique type tag for this native object. Null if not initialized.
    const void* tag();

    // A native object is treated as alive if it has been initialized and not yet been destroyed.
    bool alive();

    void* data();  // Raw pointer to the native object. Throws if object is dead.
    size_t size(); // Size of data, in bytes.

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
