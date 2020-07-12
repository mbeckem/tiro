#ifndef TIRO_VM_OBJECTS_NATIVE_HPP
#define TIRO_VM_OBJECTS_NATIVE_HPP

#include "common/span.hpp"
#include "vm/handles/global.hpp"
#include "vm/handles/handle.hpp"
#include "vm/handles/span.hpp"
#include "vm/objects/layout.hpp"
#include "vm/objects/tuple.hpp"
#include "vm/objects/value.hpp"

#include <functional>

namespace tiro::vm {

// TODO: Use a single native function type and store different kind of function pointers there.

/// A sychronous native function. Useful for wrapping simple, nonblocking native APIs.
class NativeFunction final : public HeapValue {
private:
    enum Slots {
        NameSlot,
        ValuesSlot,
        SlotCount_,
    };

    struct Payload {
        u32 params;
        NativeFunctionPtr func;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    static NativeFunction make(Context& ctx, Handle<String> name, MaybeHandle<Tuple> values,
        u32 params, NativeFunctionPtr function);

    explicit NativeFunction(Value v)
        : HeapValue(v, DebugCheck<NativeFunction>()) {}

    String name();
    Nullable<Tuple> values();
    u32 params();
    NativeFunctionPtr function();

    Layout* layout() const { return access_heap<Layout>(); }
};

class NativeFunctionFrame final {
public:
    Context& ctx() const { return ctx_; }

    Nullable<Tuple> values() const;

    size_t arg_count() const;
    Handle<Value> arg(size_t index) const;
    HandleSpan<Value> args() const { return args_; }

    void result(Value v);
    // TODO exceptions!

    NativeFunctionFrame(const NativeFunctionFrame&) = delete;
    NativeFunctionFrame& operator=(const NativeFunctionFrame&) = delete;

    explicit NativeFunctionFrame(Context& ctx, Handle<NativeFunction> function,
        HandleSpan<Value> args, MutHandle<Value> result);

private:
    Context& ctx_;
    Handle<NativeFunction> function_;
    HandleSpan<Value> args_;
    MutHandle<Value> result_;
};

/// Represents a native function that can be called to perform some async operation.
/// The coroutine will yield and wait until it is resumed by the async operation.
///
/// Note that calling functions of this type looks synchronous from the P.O.V. of
/// the user code.
class NativeAsyncFunction final : public HeapValue {
private:
    enum Slots {
        NameSlot,
        ValuesSlot,
        SlotCount_,
    };

    struct Payload {
        u32 params;
        NativeAsyncFunctionPtr func;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    static NativeAsyncFunction make(Context& ctx, Handle<String> name, MaybeHandle<Tuple> values,
        u32 params, NativeAsyncFunctionPtr function);

    explicit NativeAsyncFunction(Value v)
        : HeapValue(v, DebugCheck<NativeAsyncFunction>()) {}

    String name();
    Nullable<Tuple> values();
    u32 params();
    NativeAsyncFunctionPtr function();

    Layout* layout() const { return access_heap<Layout>(); }
};

class NativeAsyncFunctionFrame final {
public:
    Context& ctx() const { return storage().coro_.ctx(); }

    Nullable<Tuple> values() const;

    size_t arg_count() const;
    Handle<Value> arg(size_t index) const;
    void result(Value v);
    // TODO exceptions!

    NativeAsyncFunctionFrame(const NativeAsyncFunctionFrame&) = delete;
    NativeAsyncFunctionFrame& operator=(const NativeAsyncFunctionFrame&) = delete;

    NativeAsyncFunctionFrame(NativeAsyncFunctionFrame&&) noexcept = default;
    NativeAsyncFunctionFrame& operator=(NativeAsyncFunctionFrame&&) noexcept = default;

    explicit NativeAsyncFunctionFrame(Context& ctx, Handle<Coroutine> coro,
        Handle<NativeAsyncFunction> function, HandleSpan<Value> args, MutHandle<Value> result);

    ~NativeAsyncFunctionFrame();

private:
    struct Storage {
        Global<Coroutine> coro_;

        // Note: direct pointers into the stack. Only works because this kind of function
        // is a leaf function (no other functions will be called, therefore the stack will not
        // resize, therefore the pointers remain valid).
        // Note that the coroutine is being kept alive by the coro_ global handle above.
        Handle<NativeAsyncFunction> function_;
        HandleSpan<Value> args_;
        MutHandle<Value> result_;

        Storage(Context& ctx, Handle<Coroutine> coro, Handle<NativeAsyncFunction> function,
            HandleSpan<Value> args, MutHandle<Value> result);
    };

    Storage& storage() const {
        TIRO_DEBUG_ASSERT(storage_, "Invalid frame object (either moved or already resumed).");
        return *storage_;
    }

    // Schedules the coroutine for execution (after setting the return value).
    void resume();

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
        FinalizerFn cleanup = nullptr;
    };

public:
    using Layout = BufferLayout<byte, alignof(std::max_align_t), StaticPayloadPiece<Payload>>;

    static NativeObject make(Context& ctx, size_t size);

    explicit NativeObject(Value v)
        : HeapValue(v, DebugCheck<NativeObject>()) {}

    void* data() const;  // Raw pointer to the native object.
    size_t size() const; // Size of data, in bytes.

    // The function will be executed when the object is collected.
    void finalizer(FinalizerFn cleanup);
    FinalizerFn finalizer() const;

    // Calls the cleanup function. Called by the collector.
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
