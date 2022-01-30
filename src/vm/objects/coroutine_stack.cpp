#include "vm/objects/coroutine_stack.hpp"

#include "vm/context.hpp"
#include "vm/modules/verify.hpp"
#include "vm/object_support/factory.hpp"

namespace tiro::vm {

namespace {

// Safe frame handle that is not affected by heap moves.
class FrameHandle final {
public:
    FrameHandle(Handle<CoroutineStack> stack, CoroutineFrame* frame)
        : stack_(stack)
        , offset_(stack_->frame_to_offset(frame)) {}

    Handle<CoroutineStack> stack() const { return stack_; }

    CoroutineFrame* get() const { return stack_->offset_to_frame(offset_); }
    CoroutineFrame* operator->() const { return get(); }
    CoroutineFrame& operator*() const {
        auto f = get();
        TIRO_DEBUG_ASSERT(f, "invalid frame");
        return *f;
    }

    explicit operator bool() const { return get() != nullptr; }

private:
    Handle<CoroutineStack> stack_;
    u32 offset_;
};

} // namespace

std::string_view to_string(FrameType type) {
    switch (type) {
    case FrameType::Code:
        return "Code";
    case FrameType::Async:
        return "Async";
    case FrameType::Resumable:
        return "Resumable";
    case FrameType::Catch:
        return "Catch";
    }

    TIRO_UNREACHABLE("invalid frame type");
}

size_t frame_size(const CoroutineFrame* frame) {
    TIRO_DEBUG_ASSERT(frame, "invalid frame pointer");

    switch (frame->type) {
    case FrameType::Code:
        return sizeof(CodeFrame);
    case FrameType::Async:
        return sizeof(AsyncFrame);
    case FrameType::Resumable:
        return sizeof(ResumableFrame);
    case FrameType::Catch:
        return sizeof(CatchFrame);
    }

    TIRO_UNREACHABLE("invalid frame type");
}

CoroutineStack CoroutineStack::make(Context& ctx, u32 object_size) {
    return make_impl(ctx, object_size);
}

CoroutineStack
CoroutineStack::grow(Context& ctx, Handle<CoroutineStack> old_stack, u32 new_object_size) {
    [[maybe_unused]] size_t old_object_size = LayoutTraits<Layout>::dynamic_size(
        old_stack->layout());
    TIRO_DEBUG_ASSERT(
        new_object_size > old_object_size, "new stack size must be greater than the old size");

    // Copy the contents of the old stack
    CoroutineStack new_stack = make_impl(ctx, new_object_size);
    Layout* old_data = old_stack->layout();
    Layout* new_data = new_stack.layout();
    std::memcpy(new_data->data, old_data->data, old_stack->stack_used());

    // Copy properties.
    new_data->top = new_data->data + old_stack->stack_used();
    if (old_data->top_frame) {
        size_t top_frame_offset = reinterpret_cast<byte*>(old_data->top_frame) - old_data->data;
        new_data->top_frame = reinterpret_cast<CoroutineFrame*>(new_data->data + top_frame_offset);
    }
    return new_stack;
}

bool CoroutineStack::push_user_frame(
    CodeFunctionTemplate tmpl, Nullable<Environment> closure, u8 flags) {
    TIRO_DEBUG_ASSERT(top_value_count() >= tmpl.params(), "not enough arguments on the stack");

    const u32 params = tmpl.params();
    const u32 locals = tmpl.locals();
    return push_frame<CodeFrame>(flags, params, locals, tmpl, closure);
}

bool CoroutineStack::push_async_frame(NativeFunction func, u32 argc, u8 flags) {
    TIRO_DEBUG_ASSERT(top_value_count() >= argc, "not enough arguments on the stack");
    TIRO_DEBUG_ASSERT(argc >= func.params(), "not enough arguments to the call the given function");
    TIRO_DEBUG_ASSERT(func.locals() == 0, "async frames may not have locals");
    return push_frame<AsyncFrame>(flags, argc, 0, func);
}

bool CoroutineStack::push_resumable_frame(NativeFunction func, u32 argc, u8 flags) {
    TIRO_DEBUG_ASSERT(top_value_count() >= argc, "not enough arguments on the stack");
    TIRO_DEBUG_ASSERT(argc >= func.params(), "not enough arguments to the call the given function");
    return push_frame<ResumableFrame>(flags, argc, func.locals(), func);
}

bool CoroutineStack::push_catch_frame(u32 argc, u8 flags) {
    TIRO_DEBUG_ASSERT(top_value_count() >= argc, "not enough arguments on the stack");
    return push_frame<CatchFrame>(flags, argc, 0);
}

CoroutineFrame* CoroutineStack::top_frame() {
    return layout()->top_frame;
}

void CoroutineStack::pop_frame() {
    Layout* data = layout();

    TIRO_DEBUG_ASSERT(data->top_frame, "cannot pop any frames");
    data->top = reinterpret_cast<byte*>(data->top_frame);
    data->top_frame = data->top_frame->caller();
}

Value* CoroutineStack::arg(NotNull<CoroutineFrame*> frame, u32 index) {
    TIRO_DEBUG_ASSERT(index < frame->argc, "argument index out of bounds");
    return args_begin(frame) + index;
}

Span<Value> CoroutineStack::args(NotNull<CoroutineFrame*> frame) {
    return {args_begin(frame), args_end(frame)};
}

Value* CoroutineStack::local(NotNull<CoroutineFrame*> frame, u32 index) {
    TIRO_DEBUG_ASSERT(index < frame->locals, "local index out of bounds");
    return locals_begin(frame) + index;
}

Span<Value> CoroutineStack::locals(NotNull<CoroutineFrame*> frame) {
    auto begin = locals_begin(frame);
    return {begin, begin + frame->locals};
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
    TIRO_DEBUG_ASSERT(value_count(data->top_frame, data->top) > 0, "no top value");
    return values_end(data->top_frame, data->top) - 1;
}

Value* CoroutineStack::top_value(u32 n) {
    Layout* data = layout();
    TIRO_DEBUG_ASSERT(value_count(data->top_frame, data->top) > n, "no top value");
    return values_end(data->top_frame, data->top) - n - 1;
}

Span<Value> CoroutineStack::top_values(u32 n) {
    TIRO_DEBUG_ASSERT(top_value_count() >= n, "not enough values on the stack");

    Layout* data = layout();
    Value* begin = values_end(data->top_frame, data->top) - n;
    return Span<Value>(begin, static_cast<size_t>(n));
}

void CoroutineStack::pop_value() {
    Layout* data = layout();
    TIRO_DEBUG_ASSERT(data->top != (byte*) values_begin(data->top_frame), "cannot pop any values");
    data->top -= sizeof(Value);
}

void CoroutineStack::pop_values(u32 n) {
    Layout* data = layout();
    TIRO_DEBUG_ASSERT(top_value_count() >= n, "cannot pop that many values");
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

void CoroutineStack::walk(
    Context& ctx, Handle<CoroutineStack> stack, FunctionRef<void(Handle<String> name)> callback) {
    Scope sc(ctx);
    Local<String> name = sc.local<String>(defer_init);

    FrameHandle frame(stack, stack->top_frame());
    while (frame) {
        switch (frame->type) {
        case FrameType::Code:
            name = static_cast<CodeFrame*>(frame.get())->tmpl.name();
            break;
        case FrameType::Resumable:
            name = static_cast<ResumableFrame*>(frame.get())->func.name();
            break;
        case FrameType::Async:
            name = static_cast<AsyncFrame*>(frame.get())->func.name();
            break;
        case FrameType::Catch:
            name = ctx.get_interned_string("<catch panic>");
            break;
        }

        callback(name);
        frame = FrameHandle(stack, frame->caller());
    }
}

u32 CoroutineStack::frame_to_offset(CoroutineFrame* frame) {
    if (!frame)
        return std::numeric_limits<u32>::max();

    return static_cast<u32>(reinterpret_cast<byte*>(frame) - layout()->data);
}

CoroutineFrame* CoroutineStack::offset_to_frame(u32 offset) {
    if (offset == std::numeric_limits<u32>::max())
        return nullptr;

    return reinterpret_cast<CoroutineFrame*>(layout()->data + offset);
}

Value* CoroutineStack::args_begin(NotNull<CoroutineFrame*> frame) {
    return args_end(frame) - frame->argc;
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
    TIRO_DEBUG_ASSERT(layout()->top >= (byte*) values_begin(frame), "invalid top pointer");
    TIRO_DEBUG_ASSERT(static_cast<size_t>(max - layout()->data) % sizeof(Value) == 0,
        "limit not on value boundary");
    TIRO_DEBUG_ASSERT(
        (max == layout()->top || reinterpret_cast<CoroutineFrame*>(max)->caller() == frame),
        "max must either be a frame boundary or the current stack top");
    return reinterpret_cast<Value*>(max);
}

u32 CoroutineStack::value_count(CoroutineFrame* frame, byte* max) {
    return values_end(frame, max) - values_begin(frame);
}

template<typename Frame, typename... Params>
Frame*
CoroutineStack::push_frame(u8 flags, u32 argc, u32 locals, Params&&... additional_frame_params) {
    Layout* data = layout();

    void* storage = allocate_frame(sizeof(Frame), locals);
    if (!storage)
        return nullptr;

    CoroutineFrameParams params;
    params.flags = flags;
    params.argc = argc;
    params.locals = locals;
    params.caller = top_frame();

    Frame* frame = new (storage) Frame(std::forward<Params>(additional_frame_params)..., params);
    if (locals > 0) {
        Value init;
        if constexpr (std::is_same_v<CodeFrame, Frame>) {
            init = data->undef;
        } else {
            // Init to null
        }
        std::uninitialized_fill_n(reinterpret_cast<Value*>(frame + 1), locals, init);
    }
    data->top_frame = frame;
    return frame;
}

void* CoroutineStack::allocate_frame(u32 frame_size, u32 locals) {
    Layout* data = layout();
    TIRO_DEBUG_ASSERT(data->top <= data->end, "invalid stack top");
    TIRO_DEBUG_ASSERT(locals < max_locals, "too many locals");
    TIRO_DEBUG_ASSERT(locals <= (std::numeric_limits<u32>::max() - frame_size) / sizeof(Value),
        "integer overflow in frame allocation");

    const u32 required_bytes = frame_size + sizeof(Value) * locals;
    if (required_bytes > stack_available())
        return nullptr;

    byte* result = data->top;
    data->top += required_bytes;
    return result;
}

CoroutineStack CoroutineStack::make_impl(Context& ctx, u32 object_size) {
    TIRO_DEBUG_ASSERT(object_size > sizeof(Layout), "object size is too small");
    TIRO_DEBUG_ASSERT(object_size >= initial_size, "object size must be >= the inital size");

    size_t stack_size = object_size - sizeof(Layout);
    TIRO_DEBUG_ASSERT(LayoutTraits<Layout>::dynamic_alloc_size(stack_size) == object_size,
        "size calculation invariant violated");

    Layout* data = create_object<CoroutineStack>(ctx, stack_size, ctx.get_undefined(), stack_size);
    return CoroutineStack(from_heap(data));
}

} // namespace tiro::vm
