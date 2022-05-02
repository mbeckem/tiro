#include "vm/objects/native.hpp"

#include "vm/context.hpp"
#include "vm/object_support/factory.hpp"

namespace tiro::vm {

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

NativeFunctionHolder NativeFunction::function() {
    return layout()->static_payload()->function;
}

NativeFunction NativeFunction::make_impl(Context& ctx, const Builder& builder) {
    Scope sc(ctx);
    Local name = sc.local<String>(defer_init);
    if (builder.name_) {
        name = builder.name_.handle();
    } else {
        name = ctx.get_interned_string("<unnamed function>");
    }

    // TODO: Invalid value only exists because static layout requires default construction at the moment.
    TIRO_DEBUG_ASSERT(builder.holder_.valid(), "invalid native function value");

    Layout* data = create_object<NativeFunction>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(ClosureSlot, builder.closure_.to_nullable());
    data->static_payload()->params = builder.params_;
    data->static_payload()->locals = builder.locals_;
    data->static_payload()->function = builder.holder_;
    return NativeFunction(from_heap(data));
}

ResumableFrameContext::ResumableFrameContext(Context& ctx, Handle<Coroutine> coro,
    NotNull<ResumableFrame*> frame, ResumableFrameContinuation& cont)
    : ctx_(ctx)
    , coro_(coro)
    , frame_(frame)
    , cont_(cont) {
    TIRO_DEBUG_ASSERT(
        frame_ == coro->stack().value().top_frame(), "function frame must be on top the of stack");
    TIRO_DEBUG_ASSERT(cont_.action() == ResumableFrameContinuation::NONE,
        "resumable frame continuation was initialized incorrectly");
}

void ResumableFrameContinuation::do_ret(Value v) {
    action_ = RETURN;
    regs_[0].set(v);
}

void ResumableFrameContinuation::do_panic(Exception ex) {
    action_ = PANIC;
    regs_[0].set(ex);
}

void ResumableFrameContinuation::do_invoke(Function func, Nullable<Tuple> args) {
    action_ = INVOKE;
    regs_[0].set(func);
    regs_[1].set(args);
}

void ResumableFrameContinuation::do_yield() {
    action_ = YIELD;
}

ResumableFrameContinuation::RetData ResumableFrameContinuation::ret_data() const {
    TIRO_DEBUG_ASSERT(action_ == RETURN, "not a return action");
    return {regs_[0]};
}

ResumableFrameContinuation::PanicData ResumableFrameContinuation::panic_data() const {
    TIRO_DEBUG_ASSERT(action_ == PANIC, "not a panic action");
    return {regs_[0].must_cast<Exception>()};
}

ResumableFrameContinuation::InvokeData ResumableFrameContinuation::invoke_data() const {
    TIRO_DEBUG_ASSERT(action_ == INVOKE, "not an invoke action");
    return {regs_[0].must_cast<Function>(), regs_[1].must_cast<Nullable<Tuple>>()};
}

Handle<Coroutine> ResumableFrameContext::coro() const {
    return coro_;
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

MutHandle<Value> ResumableFrameContext::local(size_t index) const {
    TIRO_CHECK(index < local_count(), "local index {} is out of bounds for local count {}", index,
        local_count());
    return MutHandle<Value>::from_raw_slot(CoroutineStack::local(frame(), index));
}

MutHandleSpan<Value> ResumableFrameContext::locals() const {
    return MutHandleSpan<Value>::from_raw_slots(CoroutineStack::locals(frame()));
}

int ResumableFrameContext::state() const {
    return frame()->state;
}

void ResumableFrameContext::set_state(int state) {
    frame()->state = state;
}

void ResumableFrameContext::invoke(int next_state, Function func, Nullable<Tuple> arguments) {
    cont_.do_invoke(func, arguments);
    set_state(next_state);
}

Value ResumableFrameContext::invoke_return() {
    auto rf = frame();
    auto stack = coro_->stack().value();
    if (TIRO_UNLIKELY(stack.top_frame() != rf))
        TIRO_ERROR("the current resumable frame must be the top frame");

    u32 values = stack.top_value_count();
    TIRO_DEBUG_ASSERT(
        values == 0 || values == 1, "expected zero or one top values in resumable function frame");
    return values > 0 ? *stack.top_value() : Value::null();
}

CoroutineToken ResumableFrameContext::resume_token() {
    return Coroutine::create_token(ctx(), coro());
}

void ResumableFrameContext::yield(int next_state) {
    cont_.do_yield();
    set_state(next_state);
}

void ResumableFrameContext::return_value(Value r) {
    cont_.do_ret(r);
    set_state(ResumableFrame::END);
}

void ResumableFrameContext::panic(Exception ex) {
    cont_.do_panic(ex);
    set_state(ResumableFrame::END);
}

NotNull<ResumableFrame*> ResumableFrameContext::frame() const {
    TIRO_DEBUG_ASSERT(frame_, "invalid frame");
    return TIRO_NN(frame_);
}

AsyncResumeToken AsyncFrameContext::resume_token() {
    Context& ctx = parent_.ctx();
    UniqueExternal token(ctx.externals().allocate(parent_.resume_token()));
    return AsyncResumeToken(std::move(token));
}

UnownedAsyncResumeToken::UnownedAsyncResumeToken(External<CoroutineToken> token)
    : token_(std::move(token)) {}

Context& UnownedAsyncResumeToken::ctx() const {
    return *ExternalStorage::from_external(token_)->must_ctx();
}

void UnownedAsyncResumeToken::return_value(Value r) {
    complete(r, false);
}

void UnownedAsyncResumeToken::panic(Exception ex) {
    complete(ex, true);
}

void UnownedAsyncResumeToken::complete(Value unsafe_value, bool panic) {
    Context& ctx = get_ctx();
    Scope sc(ctx);
    Local coro = sc.local(get_coro());
    if (TIRO_UNLIKELY(ctx.interpreter().current_coroutine().same(*coro))) {
        TIRO_ERROR("invalid usage of async resume token: frame did not yield yet");
    }

    Local value = sc.local(unsafe_value);
    if (!CoroutineToken::resume(ctx, token_)) {
        TIRO_ERROR(
            "invalid usage of old async resume token: the coroutine may have resumed already");
    }

    auto frame = get_frame(coro);
    auto local = CoroutineStack::local(
        frame, panic ? AsyncFrameContext::LOCAL_PANIC : AsyncFrameContext::LOCAL_RESULT);
    *local = *value;
}

Context& UnownedAsyncResumeToken::get_ctx() {
    return *ExternalStorage::from_external(token_)->must_ctx();
}

Coroutine UnownedAsyncResumeToken::get_coro() {
    return token_->coroutine();
}

NotNull<ResumableFrame*> UnownedAsyncResumeToken::get_frame(Handle<Coroutine> coro) {
    auto stack = coro->stack();
    TIRO_DEBUG_ASSERT(!stack.is_null(), "waiting coroutines must have a stack");

    auto frame = stack.value().top_frame();
    TIRO_DEBUG_ASSERT(frame, "waiting coroutines must have a top frame");
    TIRO_DEBUG_ASSERT(
        frame->type == FrameType::Resumable, "the top frame must be a resumable frame");

    auto resumable_frame = static_cast<ResumableFrame*>(frame);
    return TIRO_NN(resumable_frame);
}

AsyncResumeToken::AsyncResumeToken(UniqueExternal<CoroutineToken> token)
    : token_(std::move(token)) {
    TIRO_DEBUG_ASSERT(token_, "invalid token");
}

AsyncResumeToken::~AsyncResumeToken() = default;

AsyncResumeToken::AsyncResumeToken(AsyncResumeToken&& other) noexcept = default;

AsyncResumeToken& AsyncResumeToken::operator=(AsyncResumeToken&& other) noexcept = default;

NativeObject NativeObject::make(Context& ctx, const tiro_native_type_t* type, size_t size) {
    TIRO_DEBUG_ASSERT(type, "invalid type");
    TIRO_DEBUG_ASSERT(is_pow2(type->alignment), "alignment must be a power of two");
    TIRO_DEBUG_ASSERT(type->alignment <= max_alignment, "alignment too large");

    Layout* data = create_object<NativeObject>(ctx, size,
        BufferInit(size, [&](Span<byte> bytes) { std::memset(bytes.begin(), 0, bytes.size()); }),
        StaticPayloadInit(), FinalizerPiece());
    TIRO_DEBUG_ASSERT(
        is_aligned(reinterpret_cast<uintptr_t>(data->buffer_begin()), type->alignment),
        "object storage is not aligned correctly");

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
