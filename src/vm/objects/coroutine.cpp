#include "vm/objects/coroutine.hpp"

#include "vm/context.hpp"
#include "vm/object_support/factory.hpp"
#include "vm/object_support/type_desc.hpp"
#include "vm/objects/result.hpp"

// #define TIRO_VM_DEBUG_COROUTINE_STATE

#ifdef TIRO_VM_DEBUG_COROUTINE_STATE
#    include <iostream>
#    define TIRO_VM_CORO_STATE(...) \
        (std::cout << "Coroutine state: " << fmt::format(__VA_ARGS__) << std::endl)
#else
#    define TIRO_VM_CORO_STATE(...)
#endif

namespace tiro::vm {

bool is_runnable(CoroutineState state) {
    return state == CoroutineState::Started || state == CoroutineState::Ready;
}

std::string_view to_string(CoroutineState state) {
    switch (state) {
    case CoroutineState::New:
        return "New";
    case CoroutineState::Started:
        return "Started";
    case CoroutineState::Ready:
        return "Ready";
    case CoroutineState::Running:
        return "Running";
    case CoroutineState::Waiting:
        return "Waiting";
    case CoroutineState::Done:
        return "Done";
    }

    TIRO_UNREACHABLE("Invalid coroutine state.");
}

std::string_view to_string(FrameType type) {
    switch (type) {
    case FrameType::Code:
        return "Code";
    case FrameType::Async:
        return "Async";
    case FrameType::Sync:
        return "Sync";
    case FrameType::Catch:
        return "Catch";
    }

    TIRO_UNREACHABLE("Invalid frame type.");
}

size_t frame_size(const CoroutineFrame* frame) {
    TIRO_DEBUG_ASSERT(frame, "Invalid frame pointer.");

    switch (frame->type) {
    case FrameType::Code:
        return sizeof(CodeFrame);
    case FrameType::Sync:
        return sizeof(SyncFrame);
    case FrameType::Async:
        return sizeof(AsyncFrame);
    case FrameType::Catch:
        return sizeof(CatchFrame);
    }

    TIRO_UNREACHABLE("Invalid frame type.");
}

CoroutineStack CoroutineStack::make(Context& ctx, u32 object_size) {
    return make_impl(ctx, object_size);
}

CoroutineStack
CoroutineStack::grow(Context& ctx, Handle<CoroutineStack> old_stack, u32 new_object_size) {
    [[maybe_unused]] size_t old_object_size = LayoutTraits<Layout>::dynamic_size(
        old_stack->layout());
    TIRO_DEBUG_ASSERT(
        new_object_size > old_object_size, "New stack size must be greater than the old size.");

    auto offset_from = [](auto* base, auto* addr) {
        return static_cast<size_t>(
            reinterpret_cast<const byte*>(addr) - reinterpret_cast<const byte*>(base));
    };

    // Copy the contents of the old stack
    CoroutineStack new_stack = make_impl(ctx, new_object_size);
    Layout* old_data = old_stack->layout();
    Layout* new_data = new_stack.layout();
    std::memcpy(new_data->data, old_data->data, old_stack->stack_used());

    // Copy properties.
    new_data->top = new_data->data + old_stack->stack_used();
    new_data->top_frame = old_data->top_frame;

    // Fixup the frame pointers (they are raw addresses and still point into the old stack).
    CoroutineFrame** frame_ptr = &new_data->top_frame;
    while (*frame_ptr) {
        size_t offset = offset_from(old_data->data, *frame_ptr);
        *frame_ptr = reinterpret_cast<CoroutineFrame*>(new_data->data + offset);
        frame_ptr = &(*frame_ptr)->caller;
    }

    return new_stack;
}

bool CoroutineStack::push_user_frame(
    CodeFunctionTemplate tmpl, Nullable<Environment> closure, u8 flags) {
    TIRO_DEBUG_ASSERT(top_value_count() >= tmpl.params(), "Not enough arguments on the stack.");

    Layout* data = layout();

    const u32 params = tmpl.params();
    const u32 locals = tmpl.locals();

    void* storage = allocate_frame(sizeof(CodeFrame), locals);
    if (!storage) {
        return false;
    }

    CodeFrame* frame = new (storage) CodeFrame(flags, params, top_frame(), tmpl, closure);
    std::uninitialized_fill_n(reinterpret_cast<Value*>(frame + 1), locals, data->undef);

    data->top_frame = frame;
    return true;
}

bool CoroutineStack::push_sync_frame(NativeFunction func, u32 argc, u8 flags) {
    TIRO_DEBUG_ASSERT(top_value_count() >= argc, "Not enough arguments on the stack.");
    TIRO_DEBUG_ASSERT(
        argc >= func.params(), "Not enough arguments to the call the given function.");

    Layout* data = layout();

    void* storage = allocate_frame(sizeof(SyncFrame), 0);
    if (!storage) {
        return false;
    }

    SyncFrame* frame = new (storage) SyncFrame(flags, argc, top_frame(), func);
    data->top_frame = frame;
    return true;
}

bool CoroutineStack::push_async_frame(NativeFunction func, u32 argc, u8 flags) {
    TIRO_DEBUG_ASSERT(top_value_count() >= argc, "Not enough arguments on the stack.");
    TIRO_DEBUG_ASSERT(
        argc >= func.params(), "Not enough arguments to the call the given function.");

    Layout* data = layout();

    void* storage = allocate_frame(sizeof(AsyncFrame), 0);
    if (!storage) {
        return false;
    }

    AsyncFrame* frame = new (storage) AsyncFrame(flags, argc, top_frame(), func);
    data->top_frame = frame;
    return true;
}

bool CoroutineStack::push_catch_frame(u32 argc, u8 flags) {
    TIRO_DEBUG_ASSERT(top_value_count() >= argc, "Not enough arguments on the stack.");

    Layout* data = layout();
    void* storage = allocate_frame(sizeof(CatchFrame), 0);
    if (!storage) {
        return false;
    }

    CatchFrame* frame = new (storage) CatchFrame(flags, argc, top_frame());
    data->top_frame = frame;
    return true;
}

CoroutineFrame* CoroutineStack::top_frame() {
    return layout()->top_frame;
}

void CoroutineStack::pop_frame() {
    Layout* data = layout();

    TIRO_DEBUG_ASSERT(data->top_frame, "Cannot pop any frames.");
    data->top = reinterpret_cast<byte*>(data->top_frame);
    data->top_frame = data->top_frame->caller;
}

Value* CoroutineStack::arg(CoroutineFrame* frame, u32 index) {
    TIRO_DEBUG_ASSERT(frame, "CoroutineStack: Invalid frame.");
    TIRO_DEBUG_ASSERT(index < frame->args, "CoroutineStack: Argument index out of bounds.");
    return args_begin(TIRO_NN(frame)) + index;
}

Span<Value> CoroutineStack::args(CoroutineFrame* frame) {
    return {args_begin(TIRO_NN(frame)), args_end(TIRO_NN(frame))};
}

Value* CoroutineStack::local(CoroutineFrame* frame, u32 index) {
    TIRO_DEBUG_ASSERT(frame, "CoroutineStack: Invalid frame.");
    TIRO_DEBUG_ASSERT(index < frame->locals, "CoroutineStack: Local index out of bounds.");
    return locals_begin(TIRO_NN(frame)) + index;
}

bool CoroutineStack::push_value(Value v) {
    Layout* data = layout();

    if (sizeof(Value) > stack_available()) {
        return false;
    }

    new (data->top) Value(v);
    data->top += sizeof(Value);
    return true;
}

u32 CoroutineStack::top_value_count() {
    Layout* data = layout();
    return value_count(data->top_frame, data->top);
}

Value* CoroutineStack::top_value() {
    Layout* data = layout();
    TIRO_DEBUG_ASSERT(value_count(data->top_frame, data->top) > 0, "No top value.");
    return values_end(data->top_frame, data->top) - 1;
}

Value* CoroutineStack::top_value(u32 n) {
    Layout* data = layout();
    TIRO_DEBUG_ASSERT(value_count(data->top_frame, data->top) > n, "No top value.");
    return values_end(data->top_frame, data->top) - n - 1;
}

Span<Value> CoroutineStack::top_values(u32 n) {
    TIRO_DEBUG_ASSERT(top_value_count() >= n, "Not enough values on the stack.");

    Layout* data = layout();
    Value* begin = values_end(data->top_frame, data->top) - n;
    return Span<Value>(begin, static_cast<size_t>(n));
}

void CoroutineStack::pop_value() {
    Layout* data = layout();
    TIRO_DEBUG_ASSERT(data->top != (byte*) values_begin(data->top_frame), "Cannot pop any values.");
    data->top -= sizeof(Value);
}

void CoroutineStack::pop_values(u32 n) {
    Layout* data = layout();
    TIRO_DEBUG_ASSERT(top_value_count() >= n, "Cannot pop that many values.");
    data->top -= sizeof(Value) * n;
}

u32 CoroutineStack::value_capacity_remaining() {
    return stack_available() / sizeof(Value);
}

u32 CoroutineStack::stack_size() {
    Layout* data = layout();
    return static_cast<u32>(data->end - data->data);
}

u32 CoroutineStack::stack_used() {
    Layout* data = layout();
    return static_cast<u32>(data->top - data->data);
}

u32 CoroutineStack::stack_available() {
    Layout* data = layout();
    return static_cast<u32>(data->end - data->top);
}

Value* CoroutineStack::args_begin(NotNull<CoroutineFrame*> frame) {
    return args_end(frame) - frame->args;
}

Value* CoroutineStack::args_end(NotNull<CoroutineFrame*> frame) {
    return reinterpret_cast<Value*>(frame.get());
}

Value* CoroutineStack::locals_begin(NotNull<CoroutineFrame*> frame) {
    byte* after_frame = reinterpret_cast<byte*>(frame.get()) + frame_size(frame);
    return reinterpret_cast<Value*>(after_frame);
}

Value* CoroutineStack::locals_end(NotNull<CoroutineFrame*> frame) {
    return locals_begin(frame) + frame->locals;
}

Value* CoroutineStack::values_begin(CoroutineFrame* frame) {
    return frame ? locals_end(NotNull(guaranteed_not_null, frame))
                 : reinterpret_cast<Value*>(layout()->data);
}

// Max points either to the start of the next frame or the end of the stack. It is always
// the pointer past-the-end for the current
Value* CoroutineStack::values_end([[maybe_unused]] CoroutineFrame* frame, byte* max) {
    TIRO_DEBUG_ASSERT(layout()->top >= (byte*) values_begin(frame), "Invalid top pointer.");
    TIRO_DEBUG_ASSERT(static_cast<size_t>(max - layout()->data) % sizeof(Value) == 0,
        "Limit not on value boundary.");
    TIRO_DEBUG_ASSERT(
        (max == layout()->top || reinterpret_cast<CoroutineFrame*>(max)->caller == frame),
        "Max must either be a frame boundary or the current stack top.");
    return reinterpret_cast<Value*>(max);
}

u32 CoroutineStack::value_count(CoroutineFrame* frame, byte* max) {
    return values_end(frame, max) - values_begin(frame);
}

void* CoroutineStack::allocate_frame(u32 frame_size, u32 locals) {
    Layout* data = layout();
    TIRO_DEBUG_ASSERT(data->top <= data->end, "Invalid stack top.");

    // TODO overflow
    const u32 required_bytes = frame_size + sizeof(Value) * locals;
    if (required_bytes > stack_available()) {
        return nullptr;
    }

    byte* result = data->top;
    data->top += required_bytes;
    return result;
}

CoroutineStack CoroutineStack::make_impl(Context& ctx, u32 object_size) {
    TIRO_DEBUG_ASSERT(object_size > sizeof(Layout), "Object size is too small.");
    TIRO_DEBUG_ASSERT(object_size >= initial_size, "Object size must be >= the inital size.");

    size_t stack_size = object_size - sizeof(Layout);
    TIRO_DEBUG_ASSERT(LayoutTraits<Layout>::dynamic_alloc_size(stack_size) == object_size,
        "Size calculation invariant violated.");

    Layout* data = create_object<CoroutineStack>(ctx, stack_size, ctx.get_undefined(), stack_size);
    return CoroutineStack(from_heap(data));
}

Coroutine Coroutine::make(Context& ctx, Handle<String> name, Handle<Value> function,
    MaybeHandle<Tuple> arguments, Handle<CoroutineStack> stack) {
    Layout* data = create_object<Coroutine>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(FunctionSlot, function);
    data->write_static_slot(ArgumentsSlot, arguments.to_nullable());
    data->write_static_slot(StackSlot, stack);
    return Coroutine(from_heap(data));
}

String Coroutine::name() {
    return layout()->read_static_slot<String>(NameSlot);
}

Value Coroutine::function() {
    return layout()->read_static_slot(FunctionSlot);
}

Nullable<Tuple> Coroutine::arguments() {
    return layout()->read_static_slot<Nullable<Tuple>>(ArgumentsSlot);
}

Nullable<CoroutineStack> Coroutine::stack() {
    return layout()->read_static_slot<Nullable<CoroutineStack>>(StackSlot);
}

void Coroutine::stack(Nullable<CoroutineStack> stack) {
    layout()->write_static_slot(StackSlot, stack);
}

Nullable<Result> Coroutine::result() {
    return layout()->read_static_slot<Nullable<Result>>(ResultSlot);
}

void Coroutine::result(Nullable<Result> result) {
    layout()->write_static_slot(ResultSlot, result);
}

CoroutineState Coroutine::state() {
    return layout()->static_payload()->state;
}

void Coroutine::state(CoroutineState state) {
#ifdef TIRO_VM_DEBUG_COROUTINE_STATE
    {
        const auto old_state = access_heap()->state;
        if (state != old_state) {
            TIRO_VM_CORO_STATE("@{} changed from {} to {}.", (void*) heap_ptr(),
                to_string(old_state), to_string(state));
        }
    }
#endif

    layout()->static_payload()->state = state;
}

Nullable<NativeObject> Coroutine::native_callback() {
    return layout()->read_static_slot<Nullable<NativeObject>>(NativeCallbackSlot);
}

void Coroutine::native_callback(Nullable<NativeObject> callback) {
    layout()->write_static_slot(NativeCallbackSlot, callback);
}

Nullable<CoroutineToken> Coroutine::current_token() {
    return layout()->read_static_slot<Nullable<CoroutineToken>>(CurrentTokenSlot);
}

void Coroutine::reset_token() {
    layout()->write_static_slot<Nullable<CoroutineToken>>(CurrentTokenSlot, {});
}

Nullable<Coroutine> Coroutine::next_ready() {
    return layout()->read_static_slot<Nullable<Coroutine>>(NextReadySlot);
}

void Coroutine::next_ready(Nullable<Coroutine> next) {
    layout()->write_static_slot(NextReadySlot, next);
}

CoroutineToken Coroutine::create_token(Context& ctx, Handle<Coroutine> coroutine) {
    if (auto current = coroutine->current_token())
        return current.value();

    // Dangling, but following code does not allocate.
    CoroutineToken token = CoroutineToken::make(ctx, coroutine);
    coroutine->layout()->write_static_slot(CurrentTokenSlot, token);
    return token;
}

void Coroutine::schedule(Context& ctx, Handle<Coroutine> coroutine) {
    TIRO_DEBUG_ASSERT(coroutine->state() == CoroutineState::Running, "Coroutine must be running.");
    ctx.resume_coroutine(coroutine);
}

CoroutineToken CoroutineToken::make(Context& ctx, Handle<Coroutine> coroutine) {
    Layout* data = create_object<CoroutineToken>(ctx, StaticSlotsInit());
    data->write_static_slot(CoroutineSlot, coroutine);
    return CoroutineToken(from_heap(data));
}

Coroutine CoroutineToken::coroutine() {
    return layout()->read_static_slot<Coroutine>(CoroutineSlot);
}

bool CoroutineToken::valid() {
    return same(coroutine().current_token());
}

bool CoroutineToken::resume(Context& ctx, Handle<CoroutineToken> token) {
    if (!token->valid())
        return false;

    Scope sc(ctx);
    Local coroutine = sc.local(token->coroutine());
    if (coroutine->state() != CoroutineState::Waiting)
        return false;

    ctx.resume_coroutine(coroutine);
    return true;
}

static void coroutine_name_impl(NativeFunctionFrame& frame) {
    auto coroutine = check_instance<Coroutine>(frame);
    frame.return_value(coroutine->name());
}

static const FunctionDesc coroutine_methods[] = {
    FunctionDesc::method("name"sv, 1, NativeFunctionStorage::static_sync<coroutine_name_impl>()),
};

const TypeDesc coroutine_type_desc{"Coroutine"sv, coroutine_methods};

static void coroutine_token_coroutine_impl(NativeFunctionFrame& frame) {
    auto token = check_instance<CoroutineToken>(frame);
    frame.return_value(token->coroutine());
}

static void coroutine_token_valid_impl(NativeFunctionFrame& frame) {
    auto token = check_instance<CoroutineToken>(frame);
    frame.return_value(frame.ctx().get_boolean(token->valid()));
}

static void coroutine_token_resume_impl(NativeFunctionFrame& frame) {
    auto token = check_instance<CoroutineToken>(frame);
    bool success = CoroutineToken::resume(frame.ctx(), token);
    frame.return_value(frame.ctx().get_boolean(success));
}

static constexpr FunctionDesc coroutine_token_methods[] = {
    FunctionDesc::method(
        "coroutine"sv, 1, NativeFunctionStorage::static_sync<coroutine_token_coroutine_impl>()),
    FunctionDesc::method(
        "valid"sv, 1, NativeFunctionStorage::static_sync<coroutine_token_valid_impl>()),
    FunctionDesc::method(
        "resume"sv, 1, NativeFunctionStorage::static_sync<coroutine_token_resume_impl>()),
};

constexpr TypeDesc coroutine_token_type_desc{"CoroutineToken"sv, coroutine_token_methods};

} // namespace tiro::vm
