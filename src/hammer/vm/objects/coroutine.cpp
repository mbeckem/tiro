#include "hammer/vm/objects/coroutine.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/coroutine.ipp"

namespace hammer::vm {

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

    HAMMER_UNREACHABLE("Invalid coroutine state.");
}

std::string_view to_string(FrameType type) {
    switch (type) {
    case FrameType::User:
        return "User";
    case FrameType::Async:
        return "Async";
    }

    HAMMER_UNREACHABLE("Invalid frame type.");
}

CoroutineStack CoroutineStack::make(Context& ctx, u32 object_size) {
    return make_impl(ctx, object_size);
}

CoroutineStack CoroutineStack::grow(
    Context& ctx, Handle<CoroutineStack> old_stack, u32 new_object_size) {
    HAMMER_ASSERT(new_object_size > old_stack->object_size(),
        "New stack size must be greater than the old size.");

    auto offset_from = [](auto* base, auto* addr) {
        return static_cast<size_t>(reinterpret_cast<const byte*>(addr)
                                   - reinterpret_cast<const byte*>(base));
    };

    // Copy the contents of the old stack
    CoroutineStack new_stack = make_impl(ctx, new_object_size);
    Data* old_data = old_stack->data();
    Data* new_data = new_stack.data();
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
    HAMMER_ASSERT(top_value_count() >= tmpl.params(),
        "Not enough arguments on the stack.");

    Data* const d = data();

    const u32 params = tmpl.params();
    const u32 locals = tmpl.locals();

    // TODO overflow
    HAMMER_ASSERT(d->top <= d->end, "Invalid stack top.");
    const size_t required_bytes = sizeof(UserFrame) + sizeof(Value) * locals;
    if (required_bytes > stack_available()) {
        return false;
    }

    UserFrame* frame = new (d->top)
        UserFrame(flags, params, top_frame(), tmpl, closure);
    std::uninitialized_fill_n(
        reinterpret_cast<Value*>(frame + 1), locals, d->undef);

    d->top_frame = frame;
    d->top += required_bytes;
    return true;
}

CoroutineFrame* CoroutineStack::top_frame() {
    return data()->top_frame;
}

void CoroutineStack::pop_frame() {
    Data* d = data();

    HAMMER_ASSERT(d->top_frame, "Cannot pop any frames.");
    d->top = reinterpret_cast<byte*>(d->top_frame);
    d->top_frame = d->top_frame->caller;
}

Value* CoroutineStack::arg(u32 index) {
    HAMMER_ASSERT(
        index < args_count(), "CoroutineStack: Argument index out of bounds.");
    return args_begin(top_frame()) + index;
}

u32 CoroutineStack::args_count() {
    auto frame = top_frame();
    HAMMER_ASSERT(frame, "CoroutineStack:: No top frame.");
    return args_end(frame) - args_begin(frame);
}

Value* CoroutineStack::local(u32 index) {
    HAMMER_ASSERT(
        index < locals_count(), "CoroutineStack: Local index out of bounds.");
    return locals_begin(top_frame()) + index;
}

u32 CoroutineStack::locals_count() {
    CoroutineFrame* frame = top_frame();
    HAMMER_ASSERT(frame, "CoroutineStack:: No top frame.");
    return locals_end(frame) - locals_begin(frame);
}

bool CoroutineStack::push_value(Value v) {
    Data* d = data();

    if (sizeof(Value) > stack_available()) {
        return false;
    }

    new (d->top) Value(v);
    d->top += sizeof(Value);
    return true;
}

u32 CoroutineStack::top_value_count() {
    Data* d = data();
    return value_count(d->top_frame, d->top);
}

Value* CoroutineStack::top_value() {
    Data* d = data();
    HAMMER_ASSERT(value_count(d->top_frame, d->top) > 0, "No top value.");
    return values_end(d->top_frame, d->top) - 1;
}

Value* CoroutineStack::top_value(u32 n) {
    Data* d = data();
    HAMMER_ASSERT(value_count(d->top_frame, d->top) > n, "No top value.");
    return values_end(d->top_frame, d->top) - n - 1;
}

Span<Value> CoroutineStack::top_values(u32 n) {
    HAMMER_ASSERT(top_value_count() >= n, "Not enough values on the stack.");

    Data* d = data();
    Value* begin = values_end(d->top_frame, d->top) - n;
    return Span<Value>(begin, static_cast<size_t>(n));
}

void CoroutineStack::pop_value() {
    Data* d = data();
    HAMMER_ASSERT(
        d->top != (byte*) values_begin(d->top_frame), "Cannot pop any values.");
    d->top -= sizeof(Value);
}

void CoroutineStack::pop_values(u32 n) {
    Data* d = data();
    HAMMER_ASSERT(top_value_count() >= n, "Cannot pop that many values.");
    d->top -= sizeof(Value) * n;
}

u32 CoroutineStack::stack_size() const noexcept {
    Data* d = data();
    return static_cast<u32>(d->end - d->data);
}

u32 CoroutineStack::stack_used() const noexcept {
    Data* d = data();
    return static_cast<u32>(d->top - d->data);
}

u32 CoroutineStack::stack_available() const noexcept {
    Data* d = data();
    return static_cast<u32>(d->end - d->top);
}

Value* CoroutineStack::args_begin(CoroutineFrame* frame) {
    HAMMER_ASSERT_NOT_NULL(frame);
    return args_end(frame) - frame->args;
}

Value* CoroutineStack::args_end(CoroutineFrame* frame) {
    HAMMER_ASSERT_NOT_NULL(frame);
    return reinterpret_cast<Value*>(frame);
}

Value* CoroutineStack::locals_begin(CoroutineFrame* frame) {
    HAMMER_ASSERT_NOT_NULL(frame);

    // TODO Uniform size?
    switch (frame->type) {
    case FrameType::User:
        return reinterpret_cast<Value*>(static_cast<UserFrame*>(frame) + 1);
    case FrameType::Async:
        HAMMER_NOT_IMPLEMENTED();
        return nullptr; // TODO
    }

    HAMMER_UNREACHABLE("Invalid frame type.");
}

Value* CoroutineStack::locals_end(CoroutineFrame* frame) {
    HAMMER_ASSERT_NOT_NULL(frame);
    return locals_begin(frame) + frame->locals;
}

Value* CoroutineStack::values_begin(CoroutineFrame* frame) {
    return frame ? locals_end(frame) : reinterpret_cast<Value*>(data()->data);
}

Value*
CoroutineStack::values_end([[maybe_unused]] CoroutineFrame* frame, byte* max) {
    HAMMER_ASSERT(
        data()->top >= (byte*) values_begin(frame), "Invalid top pointer.");
    HAMMER_ASSERT(static_cast<size_t>(max - data()->data) % sizeof(Value) == 0,
        "Limit not on value boundary.");
    HAMMER_ASSERT(
        (max == data()->top
            || reinterpret_cast<CoroutineFrame*>(max)->caller == frame),
        "Max must either be a frame boundary or the current stack top.");
    return reinterpret_cast<Value*>(max);
}

u32 CoroutineStack::value_count(CoroutineFrame* frame, byte* max) {
    return values_end(frame, max) - values_begin(frame);
}

CoroutineStack CoroutineStack::make_impl(Context& ctx, u32 object_size) {
    HAMMER_ASSERT(object_size > sizeof(Data), "Object size is too small.");
    HAMMER_ASSERT(
        object_size >= initial_size, "Object size must be >= the inital size.");

    const size_t stack_size = object_size - sizeof(Data);
    HAMMER_ASSERT((variable_allocation<Data, byte>(stack_size) == object_size),
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

String Coroutine::name() const noexcept {
    return access_heap<Data>()->name;
}

Value Coroutine::function() const {
    return access_heap<Data>()->function;
}

CoroutineStack Coroutine::stack() const noexcept {
    return access_heap<Data>()->stack;
}

void Coroutine::stack(Handle<CoroutineStack> stack) noexcept {
    access_heap<Data>()->stack = stack;
}

Value Coroutine::result() const noexcept {
    return access_heap<Data>()->result;
}

void Coroutine::result(Handle<Value> result) noexcept {
    access_heap<Data>()->result = result;
}

CoroutineState Coroutine::state() const noexcept {
    return access_heap<Data>()->state;
}

void Coroutine::state(CoroutineState state) noexcept {
    access_heap<Data>()->state = state;
}

} // namespace hammer::vm
