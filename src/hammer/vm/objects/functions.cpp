#include "hammer/vm/objects/functions.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/coroutines.hpp"

#include "hammer/vm/context.ipp"
#include "hammer/vm/objects/functions.ipp"

#include <asio/dispatch.hpp>
#include <asio/post.hpp>

// TODO Assertions vs checks

namespace hammer::vm {

Code Code::make(Context& ctx, Span<const byte> code) {
    size_t total_size = variable_allocation<Data, byte>(code.size());
    Data* data = ctx.heap().create_varsize<Data>(total_size, code);
    return Code(from_heap(data));
}

const byte* Code::data() const {
    return access_heap<Data>()->code;
}

size_t Code::size() const {
    return access_heap<Data>()->size;
}

FunctionTemplate FunctionTemplate::make(Context& ctx, Handle<String> name,
    Handle<Module> module, u32 params, u32 locals, Span<const byte> code) {
    Root<Code> code_obj(ctx, Code::make(ctx, code));

    Data* data = ctx.heap().create<Data>();
    data->name = name;
    data->module = module;
    data->params = params;
    data->locals = locals;
    data->code = code_obj;
    return FunctionTemplate(from_heap(data));
}

String FunctionTemplate::name() const {
    return access_heap<Data>()->name;
}

Module FunctionTemplate::module() const {
    return access_heap<Data>()->module;
}

Code FunctionTemplate::code() const {
    return access_heap<Data>()->code;
}

u32 FunctionTemplate::params() const {
    return access_heap<Data>()->params;
}

u32 FunctionTemplate::locals() const {
    return access_heap<Data>()->locals;
}

ClosureContext
ClosureContext::make(Context& ctx, size_t size, Handle<ClosureContext> parent) {
    HAMMER_ASSERT(size > 0, "0 sized closure context is useless.");

    size_t total_size = variable_allocation<Data, Value>(size);
    Data* data = ctx.heap().create_varsize<Data>(
        total_size, parent, ctx.get_undefined(), size);
    return ClosureContext(from_heap(data));
}

ClosureContext ClosureContext::parent() const {
    return access_heap<Data>()->parent;
}

const Value* ClosureContext::data() const {
    return access_heap<Data>()->values;
}

size_t ClosureContext::size() const {
    return access_heap<Data>()->size;
}

Value ClosureContext::get(size_t index) const {
    HAMMER_CHECK(index < size(), "ClosureContext::get(): index out of bounds.");
    return access_heap<Data>()->values[index];
}

void ClosureContext::set(size_t index, Value value) const {
    HAMMER_CHECK(index < size(), "ClosureContext::set(): index out of bounds.");
    access_heap<Data>()->values[index] = value;
}

ClosureContext ClosureContext::parent(size_t level) const {
    ClosureContext ctx = *this;
    HAMMER_ASSERT(
        !ctx.is_null(), "The current closure context cannot be null.");

    while (level != 0) {
        ctx = ctx.parent();

        if (HAMMER_UNLIKELY(ctx.is_null()))
            break;
        --level;
    }
    return ctx;
}

Function Function::make(Context& ctx, Handle<FunctionTemplate> tmpl,
    Handle<ClosureContext> closure) {
    Data* data = ctx.heap().create<Data>(tmpl, closure);
    return Function(from_heap(data));
}

FunctionTemplate Function::tmpl() const {
    return access_heap<Data>()->tmpl;
}

ClosureContext Function::closure() const {
    return access_heap<Data>()->closure;
}

BoundMethod
BoundMethod::make(Context& ctx, Handle<Value> function, Handle<Value> object) {
    HAMMER_ASSERT(function.get(), "BoundMethod::make(): Invalid function.");
    HAMMER_ASSERT(object.get(), "BoundMethod::make(): Invalid object.");

    Data* data = ctx.heap().create<Data>(function, object);
    return BoundMethod(from_heap(data));
}

Value BoundMethod::function() const {
    return access_heap()->function;
}

Value BoundMethod::object() const {
    return access_heap()->object;
}

NativeFunction NativeFunction::make(Context& ctx, Handle<String> name,
    Handle<Tuple> values, u32 min_params, FunctionType function) {
    Data* data = ctx.heap().create<Data>();
    data->name = name;
    data->values = values;
    data->min_params = min_params;
    data->func = std::move(function); // TODO use allocator from ctx
    return NativeFunction(from_heap(data));
}

String NativeFunction::name() const {
    return access_heap()->name;
}

Tuple NativeFunction::values() const {
    return access_heap()->values;
}

u32 NativeFunction::min_params() const {
    return access_heap()->min_params;
}

NativeFunction::FunctionType NativeFunction::function() const {
    return access_heap()->func;
}

NativeAsyncFunction::Frame::Storage::Storage(Context& ctx,
    Handle<Coroutine> coro, Handle<NativeAsyncFunction> function,
    Span<Value> args, MutableHandle<Value> result_slot)
    : coro_(ctx, coro.get())
    , function_(function)
    , args_(args)
    , result_slot_(result_slot) {}

NativeAsyncFunction::Frame::Frame(Context& ctx, Handle<Coroutine> coro,
    Handle<NativeAsyncFunction> function, Span<Value> args,
    MutableHandle<Value> result_slot)
    : storage_(
        std::make_unique<Storage>(ctx, coro, function, args, result_slot)) {}

NativeAsyncFunction::Frame::~Frame() {}

Tuple NativeAsyncFunction::Frame::values() const {
    return storage().function_->values();
}

size_t NativeAsyncFunction::Frame::arg_count() const {
    return storage().args_.size();
}

Handle<Value> NativeAsyncFunction::Frame::arg(size_t index) const {
    HAMMER_ASSERT(index < arg_count(),
        "NativeAsyncFunction::Frame::arg(): Index is out of bounds.");
    return Handle<Value>::from_slot(&storage().args_[index]);
}

void NativeAsyncFunction::Frame::result(Value v) {
    storage().result_slot_.set(v);
}

void NativeAsyncFunction::Frame::resume() {
    const auto coro_state = storage().coro_->state();
    if (coro_state == CoroutineState::Running) {
        // Coroutine is not yet suspended. This means that we're calling resume()
        // from the initial native function call. This is bad behaviour but we can work around
        // it by letting the coroutine suspend and then resume it in the next iteration.
        //
        // Note that the implementation below is not as efficient as is could be.
        // For example, we could have a second queue instead (in addition to the ready queue in the context).
        Context& ctx = this->ctx();
        asio::post(ctx.io_context(), [st = std::move(storage_)]() {
            // Capturing st keeps the coroutine handle alive.
            st->coro_.ctx().resume_coroutine(st->coro_);
        });
    } else if (coro_state == CoroutineState::Waiting) {
        // Coroutine has been suspended correctly, resume it now.
        //
        // dispatch() makes sure that this is safe even when called from another thread.
        Context& ctx = this->ctx();
        asio::dispatch(ctx.io_context(), [st = std::move(storage_)]() {
            st->coro_.ctx().resume_coroutine(st->coro_);
        });
    } else {
        HAMMER_ERROR("Invalid coroutine state {}, cannot resume.",
            to_string(coro_state));
    }
}

NativeAsyncFunction NativeAsyncFunction::make(Context& ctx, Handle<String> name,
    Handle<Tuple> values, u32 min_params, FunctionType function) {

    HAMMER_ASSERT(function, "Invalid function.");
    Data* data = ctx.heap().create<Data>();
    data->name = name;
    data->values = values;
    data->min_params = min_params;
    data->function = function;
    return NativeAsyncFunction(from_heap(data));
}

String NativeAsyncFunction::name() const {
    return access_heap()->name;
}

Tuple NativeAsyncFunction::values() const {
    return access_heap()->values;
}

u32 NativeAsyncFunction::min_params() const {
    return access_heap()->min_params;
}

NativeAsyncFunction::FunctionType NativeAsyncFunction::function() const {
    return access_heap()->function;
}

} // namespace hammer::vm
