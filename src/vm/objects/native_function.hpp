#ifndef TIRO_VM_OBJECTS_NATIVE_FUNCTION_HPP
#define TIRO_VM_OBJECTS_NATIVE_FUNCTION_HPP

#include "common/span.hpp"
#include "vm/heap/handles.hpp"
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

    static NativeFunction make(Context& ctx, Handle<String> name, Handle<Tuple> values, u32 params,
        NativeFunctionPtr function);

    NativeFunction() = default;

    explicit NativeFunction(Value v)
        : HeapValue(v, DebugCheck<NativeFunction>()) {}

    String name();
    Tuple values();
    u32 params();
    NativeFunctionPtr function();

    Layout* layout() const { return access_heap<Layout>(); }
};

class NativeFunctionFrame final {
public:
    Context& ctx() const { return ctx_; }

    Tuple values() const;

    size_t arg_count() const;
    Handle<Value> arg(size_t index) const;

    void result(Value v);
    // TODO exceptions!

    NativeFunctionFrame(const NativeFunctionFrame&) = delete;
    NativeFunctionFrame& operator=(const NativeFunctionFrame&) = delete;

    explicit NativeFunctionFrame(Context& ctx, Handle<NativeFunction> function,
        Span<Value> args, // TODO Must be rooted!
        MutableHandle<Value> result_slot);

private:
    Context& ctx_;
    Handle<NativeFunction> function_;
    Span<Value> args_;
    MutableHandle<Value> result_slot_;
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
    using Layout = StaticLayout<StaticSlotsPiece<2>, StaticPayloadPiece<Payload>>;

    static NativeAsyncFunction make(Context& ctx, Handle<String> name, Handle<Tuple> values,
        u32 params, NativeAsyncFunctionPtr function);

    NativeAsyncFunction() = default;

    explicit NativeAsyncFunction(Value v)
        : HeapValue(v, DebugCheck<NativeAsyncFunction>()) {}

    String name();
    Tuple values();
    u32 params();
    NativeAsyncFunctionPtr function();

    Layout* layout() const { return access_heap<Layout>(); }
};

class NativeAsyncFunctionFrame final {
public:
    Context& ctx() const { return storage().coro_.ctx(); }

    Tuple values() const;

    size_t arg_count() const;
    Handle<Value> arg(size_t index) const;
    void result(Value v);
    // TODO exceptions!

    NativeAsyncFunctionFrame(const NativeAsyncFunctionFrame&) = delete;
    NativeAsyncFunctionFrame& operator=(const NativeAsyncFunctionFrame&) = delete;

    NativeAsyncFunctionFrame(NativeAsyncFunctionFrame&&) noexcept = default;
    NativeAsyncFunctionFrame& operator=(NativeAsyncFunctionFrame&&) noexcept = default;

    explicit NativeAsyncFunctionFrame(Context& ctx, Handle<Coroutine> coro,
        Handle<NativeAsyncFunction> function,
        Span<Value> args, // TODO Must be rooted!
        MutableHandle<Value> result_slot);

    ~NativeAsyncFunctionFrame();

private:
    struct Storage {
        Global<Coroutine> coro_;

        // Note: direct pointers into the stack. Only works because this kind of function
        // is a leaf function (no other functions will be called, therefore the stack will not
        // resize, therefore the pointers remain valid).
        // Note that the coroutine is being kept alive by the coro_ global handle above.
        Handle<NativeAsyncFunction> function_;
        Span<Value> args_;
        MutableHandle<Value> result_slot_;

        Storage(Context& ctx, Handle<Coroutine> coro, Handle<NativeAsyncFunction> function,
            Span<Value> args, MutableHandle<Value> result_slot);
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
    if (!value->is<T>()) {
        TIRO_ERROR("`this` is not a {}.", to_string(TypeToTag<T>));
    }
    return value.cast<T>();
}

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_NATIVE_FUNCTION_HPP
