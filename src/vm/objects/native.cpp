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
    case NativeFunctionType::Resumable:
        return "Resumable";
    }
    TIRO_UNREACHABLE("Invalid native function type.");
}

NativeFunction NativeFunction::make(Context& ctx, Handle<String> name, MaybeHandle<Value> closure,
    u32 params, u32 locals, const NativeFunctionStorage& function) {

    // TODO: Invalid value only exists because static layout requires default construction at the moment.
    TIRO_DEBUG_ASSERT(
        function.type() != NativeFunctionType::Invalid, "invalid native function value");

    Layout* data = create_object<NativeFunction>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(ClosureSlot, closure.to_nullable());
    data->static_payload()->params = params;
    data->static_payload()->locals = locals;
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

u32 NativeFunction::locals() {
    return layout()->static_payload()->locals;
}

NativeFunctionStorage NativeFunction::function() {
    return layout()->static_payload()->function;
}

SyncFrameContext::SyncFrameContext(
    Context& ctx, Handle<Coroutine> coro, NotNull<SyncFrame*> frame, OutHandle<Value> return_value)
    : ctx_(ctx)
    , coro_(coro)
    , frame_(frame)
    , return_value_(return_value) {
    TIRO_DEBUG_ASSERT(
        frame == coro->stack().value().top_frame(), "function frame must be on top the of stack");
}

Handle<Coroutine> SyncFrameContext::coro() const {
    return coro_;
}

Value SyncFrameContext::closure() const {
    return frame_->func.closure();
}

size_t SyncFrameContext::arg_count() const {
    return frame_->argc;
}

Handle<Value> SyncFrameContext::arg(size_t index) const {
    TIRO_CHECK(index < arg_count(), "argument index {} is out of bounds for argument count {}",
        index, arg_count());
    return Handle<Value>::from_raw_slot(CoroutineStack::arg(frame_, index));
}

HandleSpan<Value> SyncFrameContext::args() const {
    return HandleSpan<Value>::from_raw_slots(CoroutineStack::args(frame_));
}

void SyncFrameContext::return_value(Value r) {
    return_value_.set(r);
    frame_->flags &= ~FRAME_UNWINDING;
}

void SyncFrameContext::panic(Exception ex) {
    return_value_.set(ex);
    frame_->flags |= FRAME_UNWINDING;
}

AsyncFrameContext::AsyncFrameContext(
    Context& ctx, Handle<Coroutine> coro, NotNull<AsyncFrame*> frame)
    : ctx_(ctx)
    , coro_external_(get_valid_slot(ctx.externals().allocate(coro)))
    , frame_(frame) {
    TIRO_DEBUG_ASSERT(
        frame == coro->stack().value().top_frame(), "function frame must be on top the of stack");
}

AsyncFrameContext::AsyncFrameContext(AsyncFrameContext&& other) noexcept
    : ctx_(other.ctx_)
    , coro_external_(std::exchange(other.coro_external_, nullptr))
    , frame_(other.frame_) {}

AsyncFrameContext::~AsyncFrameContext() {
    if (coro_external_) {
        ctx_.externals().free(External<Coroutine>::from_raw_slot(coro_external_));
    }
}

Value AsyncFrameContext::closure() const {
    return frame()->func.closure();
}

size_t AsyncFrameContext::arg_count() const {
    return frame()->argc;
}

Handle<Value> AsyncFrameContext::arg(size_t index) const {
    TIRO_CHECK(index < arg_count(), "argument index {} is out of bounds for argument count {}",
        index, arg_count());
    return Handle<Value>::from_raw_slot(CoroutineStack::arg(frame(), index));
}

HandleSpan<Value> AsyncFrameContext::args() const {
    return HandleSpan<Value>::from_raw_slots(CoroutineStack::args(frame()));
}

void AsyncFrameContext::return_value(Value v) {
    AsyncFrame* af = frame();
    af->return_value_or_exception = v;
    af->flags &= ~FRAME_UNWINDING;
    resume();
}

void AsyncFrameContext::panic(Exception ex) {
    AsyncFrame* af = frame();
    af->return_value_or_exception = ex;
    af->flags |= FRAME_UNWINDING;
    resume();
}

void AsyncFrameContext::resume() {
    Handle<Coroutine> coro = coroutine();

    // Signals to the interpreter that the a result is ready when it enters the frame again.
    AsyncFrame* af = frame();
    if (af->flags & FRAME_ASYNC_RESUMED)
        TIRO_ERROR("cannot resume a coroutine multiple times from the same async function");
    af->flags |= FRAME_ASYNC_RESUMED;

    TIRO_CHECK(coro->state() == CoroutineState::Running || coro->state() == CoroutineState::Waiting,
        "invalid coroutine state {}, cannot resume", to_string(coro->state()));

    // If state == Running:
    //      Coroutine is not yet suspended. This means that we're calling resume()
    //      from the initial native function call. This is not a problem, the interpreter will observe
    //      the RESUMED flag and continue accordingly.
    // If state == Waiting:
    //      Coroutine was suspended correctly and is now being resumed by some kind of callback.
    ctx().resume_coroutine(coro);
    frame_ = nullptr;
}

Handle<Coroutine> AsyncFrameContext::coroutine() const {
    TIRO_DEBUG_ASSERT(coro_external_ != nullptr, "async frame was moved");
    return External<Coroutine>::from_raw_slot(coro_external_);
}

NotNull<AsyncFrame*> AsyncFrameContext::frame() const {
    TIRO_DEBUG_ASSERT(coro_external_ != nullptr, "async frame was moved");
    TIRO_CHECK(frame_, "coroutine was already resumed");
    return TIRO_NN(frame_);
}

ResumableFrameContext::ResumableFrameContext(
    Context& ctx, Handle<Coroutine> coro, NotNull<ResumableFrame*> frame)
    : ctx_(ctx)
    , coro_(coro)
    , frame_(frame) {
    TIRO_DEBUG_ASSERT(
        frame_ == coro->stack().value().top_frame(), "function frame must be on top the of stack");
}

Value ResumableFrameContext::closure() const {
    return frame()->func.closure();
}

size_t ResumableFrameContext::arg_count() const {
    return frame()->argc;
}

Handle<Value> ResumableFrameContext::arg(size_t index) const {
    TIRO_CHECK(index < arg_count(), "argument index {} is out of bounds for argument count {}",
        index, arg_count());
    return Handle<Value>::from_raw_slot(CoroutineStack::arg(frame(), index));
}

HandleSpan<Value> ResumableFrameContext::args() const {
    return HandleSpan<Value>::from_raw_slots(CoroutineStack::args(frame()));
}

size_t ResumableFrameContext::local_count() const {
    return frame()->locals;
}

Handle<Value> ResumableFrameContext::local(size_t index) const {
    TIRO_CHECK(index < local_count(), "local index {} is out of bounds for local count {}", index,
        local_count());
    return Handle<Value>::from_raw_slot(CoroutineStack::local(frame(), index));
}

HandleSpan<Value> ResumableFrameContext::locals() const {
    return HandleSpan<Value>::from_raw_slots(CoroutineStack::locals(frame()));
}

int ResumableFrameContext::state() const {
    return frame()->state;
}

void ResumableFrameContext::set_state(int state) {
    frame()->state = state;
}

void ResumableFrameContext::invoke(int next_state, Value func, Nullable<Tuple> arguments) {
    auto rf = frame();
    rf->invoke_func = func;
    rf->invoke_arguments = arguments;
    rf->flags |= FRAME_RESUMABLE_INVOKE;
    set_state(next_state);
}

Value ResumableFrameContext::invoke_return() {
    auto rf = frame();
    auto stack = coro_->stack().value();
    TIRO_DEBUG_ASSERT(stack.top_frame() == rf, "current frame must be the top frame");

    u32 values = stack.top_value_count();
    TIRO_DEBUG_ASSERT(
        values == 0 || values == 1, "expected zero or one top values in resumable function frame");
    return stack.top_value_count() > 0 ? *stack.top_value() : Value::null();
}

void ResumableFrameContext::return_value(Value r) {
    auto rf = frame();
    rf->return_value_or_exception = r;
    rf->flags &= ~FRAME_UNWINDING;
    set_state(ResumableFrame::END);
}

void ResumableFrameContext::panic(Exception ex) {
    auto rf = frame();
    rf->return_value_or_exception = ex;
    rf->flags |= FRAME_UNWINDING;
    set_state(ResumableFrame::END);
}

NotNull<ResumableFrame*> ResumableFrameContext::frame() const {
    TIRO_DEBUG_ASSERT(frame_, "invalid frame");
    return TIRO_NN(frame_);
}

NativeObject NativeObject::make(Context& ctx, const tiro_native_type_t* type, size_t size) {
    Layout* data = create_object<NativeObject>(ctx, size,
        BufferInit(size, [&](Span<byte> bytes) { std::memset(bytes.begin(), 0, bytes.size()); }),
        StaticPayloadInit(), FinalizerPiece());
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
