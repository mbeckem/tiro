#include "vm/objects/native_function.hpp"

#include "vm/context.hpp"
#include "vm/objects/factory.hpp"

#include <asio/dispatch.hpp>
#include <asio/post.hpp>

namespace tiro::vm {

NativeFunction NativeFunction::make(Context& ctx, Handle<String> name, Handle<Tuple> values,
    u32 params, NativeFunctionPtr function) {

    Layout* data = create_object<NativeFunction>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(ValuesSlot, values);
    data->static_payload()->params = params;
    data->static_payload()->func = function;
    return NativeFunction(from_heap(data));
}

String NativeFunction::name() {
    return layout()->read_static_slot<String>(NameSlot);
}

Tuple NativeFunction::values() {
    return layout()->read_static_slot<Tuple>(ValuesSlot);
}

u32 NativeFunction::params() {
    return layout()->static_payload()->params;
}

NativeFunctionPtr NativeFunction::function() {
    return layout()->static_payload()->func;
}

NativeFunctionFrame::NativeFunctionFrame(Context& ctx, Handle<NativeFunction> function,
    Span<Value> args, MutableHandle<Value> result_slot)
    : ctx_(ctx)
    , function_(function)
    , args_(args)
    , result_slot_(result_slot) {}

Tuple NativeFunctionFrame::values() const {
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
    return Handle<Value>::from_slot(&args_[index]);
}

void NativeFunctionFrame::result(Value v) {
    result_slot_.set(v);
}

NativeAsyncFunction NativeAsyncFunction::make(Context& ctx, Handle<String> name,
    Handle<Tuple> values, u32 params, NativeAsyncFunctionPtr function) {
    TIRO_DEBUG_ASSERT(function, "Invalid function.");

    Layout* data = create_object<NativeAsyncFunction>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(ValuesSlot, values);
    data->static_payload()->params = params;
    data->static_payload()->func = function;
    return NativeAsyncFunction(from_heap(data));
}

String NativeAsyncFunction::name() {
    return layout()->read_static_slot<String>(NameSlot);
}

Tuple NativeAsyncFunction::values() {
    return layout()->read_static_slot<Tuple>(ValuesSlot);
}

u32 NativeAsyncFunction::params() {
    return layout()->static_payload()->params;
}

NativeAsyncFunctionPtr NativeAsyncFunction::function() {
    return layout()->static_payload()->func;
}

NativeAsyncFunctionFrame::Storage::Storage(Context& ctx, Handle<Coroutine> coro,
    Handle<NativeAsyncFunction> function, Span<Value> args, MutableHandle<Value> result_slot)
    : coro_(ctx, coro.get())
    , function_(function)
    , args_(args)
    , result_slot_(result_slot) {}

NativeAsyncFunctionFrame::NativeAsyncFunctionFrame(Context& ctx, Handle<Coroutine> coro,
    Handle<NativeAsyncFunction> function, Span<Value> args, MutableHandle<Value> result_slot)
    : storage_(std::make_unique<Storage>(ctx, coro, function, args, result_slot)) {}

NativeAsyncFunctionFrame::~NativeAsyncFunctionFrame() {}

Tuple NativeAsyncFunctionFrame::values() const {
    return storage().function_->values();
}

size_t NativeAsyncFunctionFrame::arg_count() const {
    return storage().args_.size();
}

Handle<Value> NativeAsyncFunctionFrame::arg(size_t index) const {
    TIRO_DEBUG_ASSERT(
        index < arg_count(), "NativeAsyncFunctionFrame::arg(): Index is out of bounds.");
    return Handle<Value>::from_slot(&storage().args_[index]);
}

void NativeAsyncFunctionFrame::result(Value v) {
    storage().result_slot_.set(v);
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

} // namespace tiro::vm
