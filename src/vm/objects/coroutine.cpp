#include "vm/objects/coroutine.hpp"

#include "vm/context.hpp"

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
    return state == CoroutineState::New || state == CoroutineState::Ready;
}

std::string_view to_string(CoroutineState state) {
    switch (state) {
    case CoroutineState::New:
        return "New";
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
    case FrameType::User:
        return "User";
    case FrameType::Async:
        return "Async";
    }

    TIRO_UNREACHABLE("Invalid frame type.");
}

size_t frame_size(const CoroutineFrame* frame) {
    TIRO_DEBUG_ASSERT(frame, "Invalid frame pointer.");

    switch (frame->type) {
    case FrameType::User:
        return sizeof(UserFrame);
    case FrameType::Async:
        return sizeof(AsyncFrame);
    }

    TIRO_UNREACHABLE("Invalid frame type.");
}

CoroutineStack CoroutineStack::make(Context& ctx, u32 object_size) {
    return make_impl(ctx, object_size);
}

CoroutineStack
CoroutineStack::grow(Context& ctx, Handle<CoroutineStack> old_stack, u32 new_object_size) {
    size_t old_object_size = LayoutTraits<Layout>::dynamic_size(old_stack->layout());

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

bool CoroutineStack::push_user_frame(FunctionTemplate tmpl, Environment closure, u8 flags) {
    TIRO_DEBUG_ASSERT(top_value_count() >= tmpl.params(), "Not enough arguments on the stack.");

    Layout* data = layout();

    const u32 params = tmpl.params();
    const u32 locals = tmpl.locals();

    void* storage = allocate_frame(sizeof(UserFrame), locals);
    if (!storage) {
        return false;
    }

    UserFrame* frame = new (storage) UserFrame(flags, params, top_frame(), tmpl, closure);
    std::uninitialized_fill_n(reinterpret_cast<Value*>(frame + 1), locals, data->undef);

    data->top_frame = frame;
    return true;
}

bool CoroutineStack::push_async_frame(NativeAsyncFunction func, u32 argc, u8 flags) {
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

CoroutineFrame* CoroutineStack::top_frame() {
    return layout()->top_frame;
}

void CoroutineStack::pop_frame() {
    Layout* data = layout();

    TIRO_DEBUG_ASSERT(data->top_frame, "Cannot pop any frames.");
    data->top = reinterpret_cast<byte*>(data->top_frame);
    data->top_frame = data->top_frame->caller;
}

Value* CoroutineStack::arg(u32 index) {
    TIRO_DEBUG_ASSERT(index < args_count(), "CoroutineStack: Argument index out of bounds.");
    return args_begin(top_frame()) + index;
}

u32 CoroutineStack::args_count() {
    auto frame = top_frame();
    TIRO_DEBUG_ASSERT(frame, "CoroutineStack:: No top frame.");
    return args_end(frame) - args_begin(frame);
}

Value* CoroutineStack::local(u32 index) {
    TIRO_DEBUG_ASSERT(index < locals_count(), "CoroutineStack: Local index out of bounds.");
    return locals_begin(top_frame()) + index;
}

u32 CoroutineStack::locals_count() {
    CoroutineFrame* frame = top_frame();
    TIRO_DEBUG_ASSERT(frame, "CoroutineStack:: No top frame.");
    return locals_end(frame) - locals_begin(frame);
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

Value* CoroutineStack::args_begin(CoroutineFrame* frame) {
    TIRO_DEBUG_NOT_NULL(frame);
    return args_end(frame) - frame->args;
}

Value* CoroutineStack::args_end(CoroutineFrame* frame) {
    TIRO_DEBUG_NOT_NULL(frame);
    return reinterpret_cast<Value*>(frame);
}

Value* CoroutineStack::locals_begin(CoroutineFrame* frame) {
    TIRO_DEBUG_NOT_NULL(frame);

    byte* after_frame = reinterpret_cast<byte*>(frame) + frame_size(frame);
    return reinterpret_cast<Value*>(after_frame);
}

Value* CoroutineStack::locals_end(CoroutineFrame* frame) {
    TIRO_DEBUG_NOT_NULL(frame);
    return locals_begin(frame) + frame->locals;
}

Value* CoroutineStack::values_begin(CoroutineFrame* frame) {
    return frame ? locals_end(frame) : reinterpret_cast<Value*>(layout()->data);
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
    TIRO_DEBUG_ASSERT(LayoutTraits<Layout>::dynamic_size(stack_size) == object_size,
        "Size calculation invariant violated.");

    Layout* data = ctx.heap().create_varsize<Layout>(object_size, ctx.get_undefined(), stack_size);
    return CoroutineStack(from_heap(data));
}

Coroutine Coroutine::make(Context& ctx, Handle<String> name, Handle<Value> function,
    Handle<Tuple> arguments, Handle<CoroutineStack> stack) {
    Layout* data = ctx.heap().create<Layout>(
        ValueType::Coroutine, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(NameSlot, name);
    data->write_static_slot(FunctionSlot, function);
    data->write_static_slot(ArgumentsSlot, arguments);
    data->write_static_slot(StackSlot, stack);
    return Coroutine(from_heap(data));
}

String Coroutine::name() const {
    return layout()->read_static_slot<String>(NameSlot);
}

Value Coroutine::function() const {
    return layout()->read_static_slot(FunctionSlot);
}

Tuple Coroutine::arguments() const {
    // TODO: nullable?
    return layout()->read_static_slot<Tuple>(ArgumentsSlot);
}

CoroutineStack Coroutine::stack() const {
    return layout()->read_static_slot<CoroutineStack>(StackSlot);
}

void Coroutine::stack(Handle<CoroutineStack> stack) {
    layout()->write_static_slot(StackSlot, stack);
}

Value Coroutine::result() const {
    return layout()->read_static_slot(ResultSlot);
}

void Coroutine::result(Handle<Value> result) {
    layout()->write_static_slot(ResultSlot, result);
}

CoroutineState Coroutine::state() const {
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

Coroutine Coroutine::next_ready() const {
    return layout()->read_static_slot<Coroutine>(NextReadySlot);
}

void Coroutine::next_ready(Coroutine next) {
    layout()->write_static_slot(NextReadySlot, next);
}

} // namespace tiro::vm
