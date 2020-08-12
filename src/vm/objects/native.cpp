#include "vm/objects/native.hpp"

#include "vm/context.hpp"
#include "vm/objects/factory.hpp"

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

NativeFunction NativeFunction::make(Context& ctx, Handle<String> name, MaybeHandle<Value> closure,
    u32 params, NativeFunctionPtr function) {
    TIRO_DEBUG_ASSERT(function, "Invalid function.");

    Layout* data = create_object<NativeFunction>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(ClosureSlot, closure.to_nullable());
    data->static_payload()->params = params;
    data->static_payload()->function_type = NativeFunctionType::Sync;
    data->static_payload()->sync_function = function;
    return NativeFunction(from_heap(data));
}

NativeFunction NativeFunction::make(Context& ctx, Handle<String> name, MaybeHandle<Value> closure,
    u32 params, NativeAsyncFunctionPtr function) {
    TIRO_DEBUG_ASSERT(function, "Invalid function.");

    Layout* data = create_object<NativeFunction>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(ClosureSlot, closure.to_nullable());
    data->static_payload()->params = params;
    data->static_payload()->function_type = NativeFunctionType::Async;
    data->static_payload()->async_function = function;
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

Handle<Coroutine> NativeFunctionFrame::coro() const {
    return coro_;
}

Value NativeFunctionFrame::closure() const {
    return function_->closure();
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

NativeAsyncFunctionFrame::Storage::Storage(Context& ctx, Handle<Coroutine> coro)
    : coro_(ctx, coro.get()) {}

NativeAsyncFunctionFrame::NativeAsyncFunctionFrame(Context& ctx, Handle<Coroutine> coro)
    : storage_(std::make_unique<Storage>(ctx, coro)) {
    TIRO_DEBUG_ASSERT(frame()->type == FrameType::Async, "Invalid frame type.");
}

NativeAsyncFunctionFrame::~NativeAsyncFunctionFrame() {}

Value NativeAsyncFunctionFrame::closure() const {
    return frame()->func.closure();
}

size_t NativeAsyncFunctionFrame::arg_count() const {
    return frame()->args;
}

Value NativeAsyncFunctionFrame::arg(size_t index) const {
    TIRO_DEBUG_ASSERT(
        index < arg_count(), "NativeAsyncFunctionFrame::arg(): Index is out of bounds.");
    return *stack().arg(index);
}

void NativeAsyncFunctionFrame::result(Value v) {
    frame()->return_value = v;
    resume();
}

void NativeAsyncFunctionFrame::resume() {
    Handle<Coroutine> coro = storage().coro_;

    // Signals to the interpreter that the a result is ready when it enters the frame again.
    AsyncFrame* af = frame();
    if (af->flags & FRAME_ASYNC_RESUMED)
        TIRO_ERROR("Cannot resume a coroutine multiple times from the same async function.");
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
}

CoroutineStack NativeAsyncFunctionFrame::stack() const {
    auto stack = storage().coro_->stack();
    TIRO_CHECK(stack.has_value(), "Invalid coroutine stack.");
    return stack.value();
}

AsyncFrame* NativeAsyncFunctionFrame::frame() const {
    auto stack = this->stack();
    CoroutineFrame* frame = stack.top_frame();
    TIRO_DEBUG_ASSERT(frame && frame->type == FrameType::Async,
        "Stack is corrupted, top frame must be the expected async frame.");
    return static_cast<AsyncFrame*>(frame);
}

NativeObject NativeObject::make(Context& ctx, size_t size) {
    Layout* data = create_object<NativeObject>(ctx, size,
        BufferInit(size, [&](Span<byte> bytes) { std::memset(bytes.begin(), 0, bytes.size()); }),
        StaticPayloadInit());
    return NativeObject(from_heap(data));
}

void NativeObject::construct(
    const void* tag, FunctionRef<void(void*, size_t)> constructor, FinalizerFn finalizer) {
    TIRO_CHECK(tag, "NativeObject::construct(): Must provide a valid tag.");
    TIRO_CHECK(constructor, "NativeObject::construct(): Must provide a valid constructor.");

    void* const data = layout()->buffer_begin();
    const size_t size = layout()->buffer_capacity();
    constructor(data, size);

    layout()->static_payload()->tag = tag;
    layout()->static_payload()->finalizer = finalizer;
}

void NativeObject::finalize() {
    Layout* data = layout();

    auto& tag = data->static_payload()->tag;
    auto& finalizer = data->static_payload()->finalizer;
    if (tag && finalizer) {
        finalizer(data->buffer_begin(), data->buffer_capacity());
        tag = nullptr;
        finalizer = nullptr;
    }
}

const void* NativeObject::tag() {
    return layout()->static_payload()->tag;
}

bool NativeObject::alive() {
    return tag() != nullptr;
}

void* NativeObject::data() {
    TIRO_CHECK(alive(), "NativeObject::data(): The object has already been finalized.");
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
