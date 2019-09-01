#include "hammer/vm/object.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/object.ipp"

namespace hammer::vm {

template<typename BaseType, typename ValueType>
constexpr size_t variable_allocation(size_t values) {
    // TODO these should be language level errors.

    size_t trailer = 0;
    if (HAMMER_UNLIKELY(!checked_mul(sizeof(ValueType), values, trailer)))
        HAMMER_ERROR("Allocation size overflow.");

    size_t total = 0;
    if (HAMMER_UNLIKELY(!checked_add(sizeof(BaseType), trailer, total)))
        HAMMER_ERROR("Allocation size overflow.");

    return total;
}

Null Null::make(Context&) {
    return Null(Value());
}

Undefined Undefined::make(Context& ctx) {
    Data* data = ctx.heap().create<Data>();
    return Undefined(from_heap(data));
}

Boolean Boolean::make(Context& ctx, bool value) {
    Data* data = ctx.heap().create<Data>(value);
    return Boolean(from_heap(data));
}

bool Boolean::value() const noexcept {
    return access_heap<Data>()->value;
}

Integer Integer::make(Context& ctx, i64 value) {
    Data* data = ctx.heap().create<Data>(value);
    return Integer(from_heap(data));
}

i64 Integer::value() const noexcept {
    return access_heap<Data>()->value;
}

Float Float::make(Context& ctx, double value) {
    Data* data = ctx.heap().create<Data>(value);
    return Float(from_heap(data));
}

double Float::value() const noexcept {
    return access_heap<Data>()->value;
}

String String::make(Context& ctx, std::string_view str) {
    size_t total_size = variable_allocation<Data, char>(str.size());
    Data* data = ctx.heap().create_varsize<Data>(total_size, str);
    return String(from_heap(data));
}

const char* String::data() const noexcept {
    return access_heap<Data>()->data;
}

size_t String::size() const noexcept {
    return access_heap<Data>()->size;
}

size_t String::hash() const noexcept {
    size_t& hash = access_heap<Data>()->hash;
    if (hash == 0) {
        hash = std::hash<std::string_view>()(view()) | 1;
    }
    return hash;
}

Code Code::make(Context& ctx, Span<const byte> code) {
    size_t total_size = variable_allocation<Data, byte>(code.size());
    Data* data = ctx.heap().create_varsize<Data>(total_size, code);
    return Code(from_heap(data));
}

const byte* Code::data() const noexcept {
    return access_heap<Data>()->code;
}

size_t Code::size() const noexcept {
    return access_heap<Data>()->size;
}

FunctionTemplate FunctionTemplate::make(Context& ctx, Handle<String> name,
    Handle<Module> module, Handle<Array> literals, u32 params, u32 locals,
    Span<const byte> code) {
    Root<Code> code_obj(ctx, Code::make(ctx, code));

    Data* data = ctx.heap().create<Data>();
    data->name = name;
    data->module = module;
    data->literals = literals;
    data->params = params;
    data->locals = locals;
    data->code = code_obj;
    return FunctionTemplate(from_heap(data));
}

String FunctionTemplate::name() const noexcept {
    return access_heap<Data>()->name;
}

Module FunctionTemplate::module() const noexcept {
    return access_heap<Data>()->module;
}

Array FunctionTemplate::literals() const noexcept {
    return access_heap<Data>()->literals;
}

Code FunctionTemplate::code() const noexcept {
    return access_heap<Data>()->code;
}

u32 FunctionTemplate::params() const noexcept {
    return access_heap<Data>()->params;
}

u32 FunctionTemplate::locals() const noexcept {
    return access_heap<Data>()->locals;
}

Function Function::make(
    Context& ctx, Handle<FunctionTemplate> tmpl, Handle<Value> closure) {
    Data* data = ctx.heap().create<Data>(tmpl, closure);
    return Function(from_heap(data));
}

FunctionTemplate Function::tmpl() const noexcept {
    return access_heap<Data>()->tmpl;
}

Value Function::closure() const noexcept {
    return access_heap<Data>()->closure;
}

Module Module::make(Context& ctx, Handle<String> name, Handle<Array> members) {
    Data* data = ctx.heap().create<Data>(name, members);
    return Module(from_heap(data));
}

String Module::name() const noexcept {
    return access_heap<Data>()->name;
}

Array Module::members() const noexcept {
    return access_heap<Data>()->members;
}

Array Array::make(Context& ctx, size_t size) {
    size_t total_size = variable_allocation<Data, Value>(size);
    Data* data = ctx.heap().create_varsize<Data>(total_size, size);
    return Array(from_heap(data));
}

const Value* Array::data() const noexcept {
    return access_heap<Data>()->values;
}

size_t Array::size() const noexcept {
    return access_heap<Data>()->size;
}

Value Array::get(size_t index) const noexcept {
    // TODO Exception
    HAMMER_CHECK(index < size(), "Array::get(): index out of bounds.");
    return access_heap<Data>()->values[index];
}

void Array::set(WriteBarrier, size_t index, Value value) const noexcept {
    // TODO Exception
    HAMMER_CHECK(index < size(), "Array::set(): index out of bounds.");
    access_heap<Data>()->values[index] = value;
}

CoroutineStack CoroutineStack::make(Context& ctx, u32 stack_size) {
    size_t total_size = variable_allocation<Data, byte>(stack_size);
    Data* data = ctx.heap().create_varsize<Data>(total_size, stack_size);
    return CoroutineStack(from_heap(data));
}

CoroutineStack CoroutineStack::grow(
    Context& ctx, Handle<CoroutineStack> old_stack, u32 new_size) {
    HAMMER_ASSERT(new_size > old_stack->stack_size(),
        "New stack size must be greater than the old size.");

    // Copy the contents of the old stack

    CoroutineStack new_stack = make(ctx, new_size);
    Data* old_data = old_stack->data();
    Data* new_data = new_stack.data();

    std::memcpy(new_data->data, old_data->data, old_stack->stack_used());

    auto offset_from = [](auto* base, auto* addr) {
        return static_cast<size_t>(reinterpret_cast<const byte*>(addr)
                                   - reinterpret_cast<const byte*>(base));
    };

    // Copy properties.
    new_data->top = new_data->data + old_stack->stack_used();
    new_data->top_frame = old_data->top_frame;

    // Fixup the frame pointers (they are raw addresses and still point into the old stack).
    Frame** frame_ptr = &new_data->top_frame;
    while (*frame_ptr) {
        size_t offset = offset_from(old_data->data, *frame_ptr);
        *frame_ptr = reinterpret_cast<Frame*>(new_data->data + offset);
        frame_ptr = &(*frame_ptr)->caller;
    }

    return new_stack;
}

bool CoroutineStack::push_frame(FunctionTemplate tmpl, Value closure) {
    HAMMER_ASSERT(!tmpl.is_null(), "Function template cannot be null.");
    HAMMER_ASSERT(top_value_count() >= tmpl.params(),
        "Not enough arguments on the stack.");

    Data* d = data();

    // TODO overflow
    HAMMER_ASSERT(d->top <= d->end, "Invalid stack top.");
    const size_t required_bytes = sizeof(Frame) + sizeof(Value) * tmpl.locals();
    if (required_bytes > stack_available()) {
        return false;
    }

    Frame* frame = new (d->top) Frame();
    frame->caller = top_frame();
    frame->tmpl = tmpl;
    frame->closure = closure;
    frame->args = tmpl.params();
    frame->locals = tmpl.locals();
    frame->pc = tmpl.code().data();

    // TODO guarantee through static analysis that no uninitialized reads can happen.
    // TODO special value for uninitialized
    std::uninitialized_fill_n(reinterpret_cast<Value*>(frame + 1),
        frame->locals, Value::null() /* TODO uninitialized */);

    d->top_frame = frame;
    d->top += required_bytes;
    return true;
}

CoroutineStack::Frame* CoroutineStack::top_frame() {
    return data()->top_frame;
}

void CoroutineStack::pop_frame() {
    Data* d = data();

    HAMMER_ASSERT(d->top_frame, "Cannot pop any frames.");
    d->top = reinterpret_cast<byte*>(d->top_frame);
    d->top_frame = d->top_frame->caller;
}

Span<Value> CoroutineStack::args() {
    Frame* frame = top_frame();
    HAMMER_ASSERT(frame, "No top frame.");
    return {args_begin(frame), args_end(frame)};
}

Span<Value> CoroutineStack::locals() {
    Frame* frame = top_frame();
    HAMMER_ASSERT(frame, "No top frame.");
    return {locals_begin(frame), locals_end(frame)};
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

Value* CoroutineStack::args_begin(Frame* frame) {
    HAMMER_ASSERT_NOT_NULL(frame);
    return args_end(frame) - frame->args;
}

Value* CoroutineStack::args_end(Frame* frame) {
    HAMMER_ASSERT_NOT_NULL(frame);
    return reinterpret_cast<Value*>(frame);
}

Value* CoroutineStack::locals_begin(Frame* frame) {
    HAMMER_ASSERT_NOT_NULL(frame);
    return reinterpret_cast<Value*>(frame + 1);
}

Value* CoroutineStack::locals_end(Frame* frame) {
    HAMMER_ASSERT_NOT_NULL(frame);
    return locals_begin(frame) + frame->locals;
}

Value* CoroutineStack::values_begin(Frame* frame) {
    return frame ? locals_end(frame) : reinterpret_cast<Value*>(data()->data);
}

Value* CoroutineStack::values_end(Frame* frame, byte* max) {
    HAMMER_ASSERT(
        data()->top >= (byte*) values_begin(frame), "Invalid top pointer.");
    HAMMER_ASSERT(static_cast<size_t>(max - data()->data) % sizeof(Value) == 0,
        "Limit not on value boundary.");
    unused(frame);
    return reinterpret_cast<Value*>(max);
}

u32 CoroutineStack::value_count(Frame* frame, byte* max) {
    return values_end(frame, max) - values_begin(frame);
}

Coroutine Coroutine::make(
    Context& ctx, Handle<String> name, Handle<CoroutineStack> stack) {
    Data* data = ctx.heap().create<Data>(name, stack);
    return Coroutine(from_heap(data));
}

String Coroutine::name() const noexcept {
    return access_heap<Data>()->name;
}

CoroutineStack Coroutine::stack() const noexcept {
    return access_heap<Data>()->stack;
}

void Coroutine::stack(WriteBarrier, Handle<CoroutineStack> stack) noexcept {
    access_heap<Data>()->stack = stack;
}

Value Coroutine::result() const noexcept {
    return access_heap<Data>()->result;
}

void Coroutine::result(WriteBarrier, Handle<Value> result) noexcept {
    access_heap<Data>()->result = result;
}

CoroutineState Coroutine::state() const noexcept {
    return access_heap<Data>()->state;
}

void Coroutine::state(CoroutineState state) noexcept {
    access_heap<Data>()->state = state;
}

} // namespace hammer::vm
