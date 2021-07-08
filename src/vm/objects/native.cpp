#include "vm/objects/native.hpp"

#include "vm/context.hpp"
#include "vm/object_support/factory.hpp"

namespace tiro::vm {

std::string_view to_string(NativeFunctionType type) {
    switch (type) {
    case NativeFunctionType::Invalid:
        return "Invalid";
    case NativeFunctionType::Sync:
        return "Sync";
    case NativeFunctionType::Async:
        return "Async";
    }
    TIRO_UNREACHABLE("Invalid native function type.");
}

NativeFunction NativeFunction::make(Context& ctx, Handle<String> name, MaybeHandle<Value> closure,
    u32 params, const NativeFunctionStorage& function) {

    // TODO: Invalid value only exists because static layout requires default construction at the moment.
    TIRO_CHECK(function.type() != NativeFunctionType::Invalid,
        "Invalid native function values are not allowed.");

    Layout* data = create_object<NativeFunction>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(ClosureSlot, closure.to_nullable());
    data->static_payload()->params = params;
    data->static_payload()->function = function;
    return NativeFunction(from_heap(data));
}

String NativeFunction::name() {
    return layout()->read_static_slot<String>(NameSlot);
}

Value NativeFunction::closure() {
    return layout()->read_static_slot<Value>(ClosureSlot);
}

u32 NativeFunction::params() {
    return layout()->static_payload()->params;
}

NativeFunctionStorage NativeFunction::function() {
    return layout()->static_payload()->function;
}

NativeFunctionFrame::NativeFunctionFrame(
    Context& ctx, Handle<Coroutine> coro, SyncFrame* frame, OutHandle<Value> return_value)
    : ctx_(ctx)
    , coro_(coro)
    , frame_(frame)
    , return_value_(return_value) {
    TIRO_DEBUG_ASSERT(frame, "Invalid frame.");
    TIRO_DEBUG_ASSERT(
        frame == coro->stack().value().top_frame(), "Function frame must be on top the of stack.");
}

Handle<Coroutine> NativeFunctionFrame::coro() const {
    return coro_;
}

Value NativeFunctionFrame::closure() const {
    return frame_->func.closure();
}

size_t NativeFunctionFrame::arg_count() const {
    return frame_->args;
}

Handle<Value> NativeFunctionFrame::arg(size_t index) const {
    TIRO_CHECK(index < arg_count(),
        "NativeFunctionFrame::arg(): Index {} is out of bounds for "
        "argument count {}.",
        index, arg_count());
    return Handle<Value>::from_raw_slot(CoroutineStack::arg(frame_, index));
}

HandleSpan<Value> NativeFunctionFrame::args() const {
    return HandleSpan<Value>::from_raw_slots(CoroutineStack::args(frame_));
}

void NativeFunctionFrame::return_value(Value r) {
    return_value_.set(r);
    frame_->flags &= ~FRAME_UNWINDING;
}

void NativeFunctionFrame::panic(Value ex) {
    return_value_.set(ex);
    frame_->flags |= FRAME_UNWINDING;
}

NativeAsyncFunctionFrame::NativeAsyncFunctionFrame(
    Context& ctx, Handle<Coroutine> coro, AsyncFrame* frame)
    : ctx_(ctx)
    , coro_external_(get_valid_slot(ctx.externals().allocate(coro)))
    , frame_(frame) {
    TIRO_DEBUG_ASSERT(frame, "Invalid frame.");
    TIRO_DEBUG_ASSERT(
        frame == coro->stack().value().top_frame(), "Function frame must be on top the of stack.");
}

NativeAsyncFunctionFrame::NativeAsyncFunctionFrame(NativeAsyncFunctionFrame&& other) noexcept
    : ctx_(other.ctx_)
    , coro_external_(std::exchange(other.coro_external_, nullptr))
    , frame_(other.frame_) {}

NativeAsyncFunctionFrame::~NativeAsyncFunctionFrame() {
    if (coro_external_) {
        ctx_.externals().free(External<Coroutine>::from_raw_slot(coro_external_));
    }
}

Value NativeAsyncFunctionFrame::closure() const {
    return frame()->func.closure();
}

size_t NativeAsyncFunctionFrame::arg_count() const {
    return frame()->args;
}

Handle<Value> NativeAsyncFunctionFrame::arg(size_t index) const {
    TIRO_CHECK(index < arg_count(),
        "NativeAsyncFunctionFrame::arg(): Index {} is out of bounds for "
        "argument count {}.",
        index, arg_count());
    return Handle<Value>::from_raw_slot(CoroutineStack::arg(frame(), index));
}

HandleSpan<Value> NativeAsyncFunctionFrame::args() const {
    return HandleSpan<Value>::from_raw_slots(CoroutineStack::args(frame()));
}

void NativeAsyncFunctionFrame::return_value(Value v) {
    frame()->return_value = v;
    resume();
}

void NativeAsyncFunctionFrame::resume() {
    Handle<Coroutine> coro = coroutine();

    // Signals to the interpreter that the a result is ready when it enters the frame again.
    AsyncFrame* af = frame();
    if (af->flags & FRAME_ASYNC_RESUMED)
        TIRO_ERROR("cannot resume a coroutine multiple times from the same async function");
    af->flags |= FRAME_ASYNC_RESUMED;

    TIRO_CHECK(coro->state() == CoroutineState::Running || coro->state() == CoroutineState::Waiting,
        "Invalid coroutine state {}, cannot resume.", to_string(coro->state()));

    // If state == Running:
    //      Coroutine is not yet suspended. This means that we're calling resume()
    //      from the initial native function call. This is not a problem, the interpreter will observe
    //      the RESUMED flag and continue accordingly.
    // If state == Waiting:
    //      Coroutine was suspended correctly and is now being resumed by some kind of callback.
    ctx().resume_coroutine(coro);
    frame_ = nullptr;
}

Handle<Coroutine> NativeAsyncFunctionFrame::coroutine() const {
    TIRO_DEBUG_ASSERT(coro_external_ != nullptr, "Async frame was moved.");
    return External<Coroutine>::from_raw_slot(coro_external_);
}

AsyncFrame* NativeAsyncFunctionFrame::frame() const {
    TIRO_DEBUG_ASSERT(coro_external_ != nullptr, "Async frame was moved.");
    TIRO_CHECK(frame_, "Coroutine was already resumed.");
    return frame_;
}

NativeObject NativeObject::make(Context& ctx, const tiro_native_type_t* type, size_t size) {
    Layout* data = create_object<NativeObject>(ctx, size,
        BufferInit(size, [&](Span<byte> bytes) { std::memset(bytes.begin(), 0, bytes.size()); }),
        StaticPayloadInit());
    data->static_payload()->type = type;
    return NativeObject(from_heap(data));
}

void NativeObject::finalize() {
    Layout* data = layout();

    auto native_type = data->static_payload()->type;
    auto finalizer = native_type->finalizer;
    if (finalizer) {
        finalizer(data->buffer_begin(), data->buffer_capacity());
    }
}

const tiro_native_type_t* NativeObject::native_type() {
    return layout()->static_payload()->type;
}

void* NativeObject::data() {
    return layout()->buffer_begin();
}

size_t NativeObject::size() {
    return layout()->buffer_capacity();
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
