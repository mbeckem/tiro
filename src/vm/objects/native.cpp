#include "vm/objects/native.hpp"

#include "vm/context.hpp"
#include "vm/objects/factory.hpp"

#include <asio/dispatch.hpp>
#include <asio/post.hpp>

namespace tiro::vm {

std::string_view to_string(NativeFunctionType type) {
    switch (type) {
    case NativeFunctionType::Sync:
        return "Sync";
    case NativeFunctionType::Async:
        return "Async";
    }
    TIRO_UNREACHABLE("Invalid native function type.");
}

NativeFunction NativeFunction::make(Context& ctx, Handle<String> name, MaybeHandle<Tuple> values,
    u32 params, NativeFunctionPtr function) {
    TIRO_DEBUG_ASSERT(function, "Invalid function.");

    Layout* data = create_object<NativeFunction>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(ValuesSlot, values.to_nullable());
    data->static_payload()->params = params;
    data->static_payload()->function_type = NativeFunctionType::Sync;
    data->static_payload()->sync_function = function;
    return NativeFunction(from_heap(data));
}

NativeFunction NativeFunction::make(Context& ctx, Handle<String> name, MaybeHandle<Tuple> values,
    u32 params, NativeAsyncFunctionPtr function) {
    TIRO_DEBUG_ASSERT(function, "Invalid function.");

    Layout* data = create_object<NativeFunction>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(ValuesSlot, values.to_nullable());
    data->static_payload()->params = params;
    data->static_payload()->function_type = NativeFunctionType::Async;
    data->static_payload()->async_function = function;
    return NativeFunction(from_heap(data));
}

String NativeFunction::name() {
    return layout()->read_static_slot<String>(NameSlot);
}

Nullable<Tuple> NativeFunction::values() {
    return layout()->read_static_slot<Nullable<Tuple>>(ValuesSlot);
}

u32 NativeFunction::params() {
    return layout()->static_payload()->params;
}

NativeFunctionType NativeFunction::function_type() {
    return layout()->static_payload()->function_type;
}

NativeFunctionPtr NativeFunction::sync_function() {
    TIRO_CHECK(function_type() == NativeFunctionType::Sync,
        "NativeFunction: invalid cast to sync function.");
    return layout()->static_payload()->sync_function;
}

NativeAsyncFunctionPtr NativeFunction::async_function() {
    TIRO_CHECK(function_type() == NativeFunctionType::Async,
        "NativeFunction: invalid cast to async function.");
    return layout()->static_payload()->async_function;
}

NativeFunctionFrame::NativeFunctionFrame(Context& ctx, Handle<Coroutine> coro,
    Handle<NativeFunction> function, HandleSpan<Value> args, MutHandle<Value> result)
    : ctx_(ctx)
    , coro_(coro)
    , function_(function)
    , args_(args)
    , result_(result) {}

Nullable<Tuple> NativeFunctionFrame::values() const {
    return function_->values();
}

size_t NativeFunctionFrame::arg_count() const {
    return args_.size();
}

Handle<Value> NativeFunctionFrame::arg(size_t index) const {
    TIRO_CHECK(index < args_.size(),
        "NativeFunction::Frame::arg(): Index {} is out of bounds for "
        "argument count {}.",
        index, args_.size());
    return args_[index];
}

void NativeFunctionFrame::result(Value v) {
    result_.set(v);
}

NativeAsyncFunctionFrame::Storage::Storage(Context& ctx, Handle<Coroutine> coro,
    Handle<NativeFunction> function, HandleSpan<Value> args, MutHandle<Value> result)
    : coro_(ctx, coro.get())
    , function_(function)
    , args_(args)
    , result_(result) {}

NativeAsyncFunctionFrame::NativeAsyncFunctionFrame(Context& ctx, Handle<Coroutine> coro,
    Handle<NativeFunction> function, HandleSpan<Value> args, MutHandle<Value> result)
    : storage_(std::make_unique<Storage>(ctx, coro, function, args, result)) {}

NativeAsyncFunctionFrame::~NativeAsyncFunctionFrame() {}

Nullable<Tuple> NativeAsyncFunctionFrame::values() const {
    return storage().function_->values();
}

size_t NativeAsyncFunctionFrame::arg_count() const {
    return storage().args_.size();
}

Handle<Value> NativeAsyncFunctionFrame::arg(size_t index) const {
    TIRO_DEBUG_ASSERT(
        index < arg_count(), "NativeAsyncFunctionFrame::arg(): Index is out of bounds.");
    return storage().args_[index];
}

void NativeAsyncFunctionFrame::result(Value v) {
    storage().result_.set(v);
    resume();
}

void NativeAsyncFunctionFrame::resume() {
    const auto coro_state = storage().coro_->state();
    if (coro_state == CoroutineState::Running) {
        // Coroutine is not yet suspended. This means that we're calling resume()
        // from the initial native function call. This is bad behaviour but we can work around
        // it by letting the coroutine suspend and then resume it in the next iteration.
        //
        // Note that the implementation below is not as efficient as is could be.
        // For example, we could have a second queue instead (in addition to the ready queue in the context).
        Context& ctx = this->ctx();
        asio::post(ctx.io_context(), [st = std::move(storage_)]() {
            // Capturing st keeps the coroutine handle alive.
            st->coro_.ctx().resume_coroutine(st->coro_);
        });
    } else if (coro_state == CoroutineState::Waiting) {
        // Coroutine has been suspended correctly, resume it now.
        //
        // dispatch() makes sure that this is safe even when called from another thread.
        Context& ctx = this->ctx();
        asio::dispatch(ctx.io_context(),
            [st = std::move(storage_)]() { st->coro_.ctx().resume_coroutine(st->coro_); });
    } else {
        TIRO_ERROR("Invalid coroutine state {}, cannot resume.", to_string(coro_state));
    }
}

NativeObject NativeObject::make(Context& ctx, size_t size) {
    Layout* data = create_object<NativeObject>(ctx, size,
        BufferInit(size, [&](Span<byte> bytes) { std::memset(bytes.begin(), 0, bytes.size()); }),
        StaticPayloadInit());
    return NativeObject(from_heap(data));
}

void* NativeObject::data() const {
    return layout()->buffer_begin();
}

size_t NativeObject::size() const {
    return layout()->buffer_capacity();
}

void NativeObject::finalizer(FinalizerFn cleanup) {
    layout()->static_payload()->cleanup = cleanup;
}

NativeObject::FinalizerFn NativeObject::finalizer() const {
    return layout()->static_payload()->cleanup;
}

void NativeObject::finalize() {
    Layout* data = layout();
    if (data->static_payload()->cleanup) {
        data->static_payload()->cleanup(data->buffer_begin(), data->buffer_capacity());
    }
}

NativePointer NativePointer::make(Context& ctx, void* ptr) {
    Layout* data = create_object<NativePointer>(ctx, StaticPayloadInit());
    data->static_payload()->ptr = ptr;
    return NativePointer(from_heap(data));
}

void* NativePointer::data() const {
    return layout()->static_payload()->ptr;
}

} // namespace tiro::vm