#include "tiro/objects/coroutines.hpp"

#include "tiro/objects/coroutines.ipp"
#include "tiro/vm/context.hpp"

// #define TIRO_VM_DEBUG_COROUTINE_STATE

#ifdef TIRO_VM_DEBUG_COROUTINE_STATE
#    include <iostream>
#    define TIRO_VM_CORO_STATE(...)                                   \
        (std::cout << "Coroutine state: " << fmt::format(__VA_ARGS__) \
                   << std::endl)
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
    TIRO_ASSERT(frame, "Invalid frame pointer.");

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

CoroutineStack CoroutineStack::grow(
    Context& ctx, Handle<CoroutineStack> old_stack, u32 new_object_size) {
    TIRO_ASSERT(new_object_size > old_stack->object_size(),
        "New stack size must be greater than the old size.");

    auto offset_from = [](auto* base, auto* addr) {
        return static_cast<size_t>(reinterpret_cast<const byte*>(addr)
                                   - reinterpret_cast<const byte*>(base));
    };

    // Copy the contents of the old stack
    CoroutineStack new_stack = make_impl(ctx, new_object_size);
    Data* old_data = old_stack->access_heap();
    Data* new_data = new_stack.access_heap();
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
    FunctionTemplate tmpl, ClosureContext closure, u8 flags) {
    TIRO_ASSERT(top_value_count() >= tmpl.params(),
        "Not enough arguments on the stack.");

    Data* const d = access_heap();

    const u32 params = tmpl.params();
    const u32 locals = tmpl.locals();

    void* storage = allocate_frame(sizeof(UserFrame), locals);
    if (!storage) {
        return false;
    }

    UserFrame* frame = new (storage)
        UserFrame(flags, params, top_frame(), tmpl, closure);
    std::uninitialized_fill_n(
        reinterpret_cast<Value*>(frame + 1), locals, d->undef);

    d->top_frame = frame;
    return true;
}

bool CoroutineStack::push_async_frame(
    NativeAsyncFunction func, u32 argc, u8 flags) {
    TIRO_ASSERT(
        top_value_count() >= argc, "Not enough arguments on the stack.");
    TIRO_ASSERT(argc >= func.params(),
        "Not enough arguments to the call the given function.");

    Data* const d = access_heap();

    void* storage = allocate_frame(sizeof(AsyncFrame), 0);
    if (!storage) {
        return false;
    }

    AsyncFrame* frame = new (storage)
        AsyncFrame(flags, argc, top_frame(), func);
    d->top_frame = frame;
    return true;
}

CoroutineFrame* CoroutineStack::top_frame() {
    return access_heap()->top_frame;
}

void CoroutineStack::pop_frame() {
    Data* d = access_heap();

    TIRO_ASSERT(d->top_frame, "Cannot pop any frames.");
    d->top = reinterpret_cast<byte*>(d->top_frame);
    d->top_frame = d->top_frame->caller;
}

Value* CoroutineStack::arg(u32 index) {
    TIRO_ASSERT(
        index < args_count(), "CoroutineStack: Argument index out of bounds.");
    return args_begin(top_frame()) + index;
}

u32 CoroutineStack::args_count() {
    auto frame = top_frame();
    TIRO_ASSERT(frame, "CoroutineStack:: No top frame.");
    return args_end(frame) - args_begin(frame);
}

Value* CoroutineStack::local(u32 index) {
    TIRO_ASSERT(
        index < locals_count(), "CoroutineStack: Local index out of bounds.");
    return locals_begin(top_frame()) + index;
}

u32 CoroutineStack::locals_count() {
    CoroutineFrame* frame = top_frame();
    TIRO_ASSERT(frame, "CoroutineStack:: No top frame.");
    return locals_end(frame) - locals_begin(frame);
}

bool CoroutineStack::push_value(Value v) {
    Data* d = access_heap();

    if (sizeof(Value) > stack_available()) {
        return false;
    }

    new (d->top) Value(v);
    d->top += sizeof(Value);
    return true;
}

u32 CoroutineStack::top_value_count() {
    Data* d = access_heap();
    return value_count(d->top_frame, d->top);
}

Value* CoroutineStack::top_value() {
    Data* d = access_heap();
    TIRO_ASSERT(value_count(d->top_frame, d->top) > 0, "No top value.");
    return values_end(d->top_frame, d->top) - 1;
}

Value* CoroutineStack::top_value(u32 n) {
    Data* d = access_heap();
    TIRO_ASSERT(value_count(d->top_frame, d->top) > n, "No top value.");
    return values_end(d->top_frame, d->top) - n - 1;
}

Span<Value> CoroutineStack::top_values(u32 n) {
    TIRO_ASSERT(top_value_count() >= n, "Not enough values on the stack.");

    Data* d = access_heap();
    Value* begin = values_end(d->top_frame, d->top) - n;
    return Span<Value>(begin, static_cast<size_t>(n));
}

void CoroutineStack::pop_value() {
    Data* d = access_heap();
    TIRO_ASSERT(
        d->top != (byte*) values_begin(d->top_frame), "Cannot pop any values.");
    d->top -= sizeof(Value);
}

void CoroutineStack::pop_values(u32 n) {
    Data* d = access_heap();
    TIRO_ASSERT(top_value_count() >= n, "Cannot pop that many values.");
    d->top -= sizeof(Value) * n;
}

u32 CoroutineStack::value_capacity_remaining() const {
    return stack_available() / sizeof(Value);
}

u32 CoroutineStack::stack_size() const {
    Data* d = access_heap();
    return static_cast<u32>(d->end - d->data);
}

u32 CoroutineStack::stack_used() const {
    Data* d = access_heap();
    return static_cast<u32>(d->top - d->data);
}

u32 CoroutineStack::stack_available() const {
    Data* d = access_heap();
    return static_cast<u32>(d->end - d->top);
}

Value* CoroutineStack::args_begin(CoroutineFrame* frame) {
    TIRO_ASSERT_NOT_NULL(frame);
    return args_end(frame) - frame->args;
}

Value* CoroutineStack::args_end(CoroutineFrame* frame) {
    TIRO_ASSERT_NOT_NULL(frame);
    return reinterpret_cast<Value*>(frame);
}

Value* CoroutineStack::locals_begin(CoroutineFrame* frame) {
    TIRO_ASSERT_NOT_NULL(frame);

    byte* after_frame = reinterpret_cast<byte*>(frame) + frame_size(frame);
    return reinterpret_cast<Value*>(after_frame);
}

Value* CoroutineStack::locals_end(CoroutineFrame* frame) {
    TIRO_ASSERT_NOT_NULL(frame);
    return locals_begin(frame) + frame->locals;
}

Value* CoroutineStack::values_begin(CoroutineFrame* frame) {
    return frame ? locals_end(frame)
                 : reinterpret_cast<Value*>(access_heap()->data);
}

Value*
CoroutineStack::values_end([[maybe_unused]] CoroutineFrame* frame, byte* max) {
    TIRO_ASSERT(access_heap()->top >= (byte*) values_begin(frame),
        "Invalid top pointer.");
    TIRO_ASSERT(
        static_cast<size_t>(max - access_heap()->data) % sizeof(Value) == 0,
        "Limit not on value boundary.");
    TIRO_ASSERT((max == access_heap()->top
                    || reinterpret_cast<CoroutineFrame*>(max)->caller == frame),
        "Max must either be a frame boundary or the current stack top.");
    return reinterpret_cast<Value*>(max);
}

u32 CoroutineStack::value_count(CoroutineFrame* frame, byte* max) {
    return values_end(frame, max) - values_begin(frame);
}

void* CoroutineStack::allocate_frame(u32 frame_size, u32 locals) {
    Data* const d = access_heap();
    TIRO_ASSERT(d->top <= d->end, "Invalid stack top.");

    // TODO overflow
    const u32 required_bytes = frame_size + sizeof(Value) * locals;
    if (required_bytes > stack_available()) {
        return nullptr;
    }

    byte* result = d->top;
    d->top += required_bytes;
    return result;
}

CoroutineStack CoroutineStack::make_impl(Context& ctx, u32 object_size) {
    TIRO_ASSERT(object_size > sizeof(Data), "Object size is too small.");
    TIRO_ASSERT(
        object_size >= initial_size, "Object size must be >= the inital size.");

    const size_t stack_size = object_size - sizeof(Data);
    TIRO_ASSERT((variable_allocation<Data, byte>(stack_size) == object_size),
        "Size calculation invariant violated.");

    Data* data = ctx.heap().create_varsize<Data>(
        object_size, ctx.get_undefined(), stack_size);
    return CoroutineStack(from_heap(data));
}

Coroutine Coroutine::make(Context& ctx, Handle<String> name,
    Handle<Value> function, Handle<CoroutineStack> stack) {
    Data* data = ctx.heap().create<Data>(name, function, stack);
    return Coroutine(from_heap(data));
}

String Coroutine::name() const {
    return access_heap()->name;
}

Value Coroutine::function() const {
    return access_heap()->function;
}

CoroutineStack Coroutine::stack() const {
    return access_heap()->stack;
}

void Coroutine::stack(Handle<CoroutineStack> stack) {
    access_heap()->stack = stack;
}

Value Coroutine::result() const {
    return access_heap()->result;
}

void Coroutine::result(Handle<Value> result) {
    access_heap()->result = result;
}

CoroutineState Coroutine::state() const {
    return access_heap()->state;
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

    access_heap()->state = state;
}

Coroutine Coroutine::next_ready() const {
    return access_heap()->next_ready;
}

void Coroutine::next_ready(Coroutine next) {
    access_heap()->next_ready = next;
}

} // namespace tiro::vm
