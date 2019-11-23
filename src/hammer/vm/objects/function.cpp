#include "hammer/vm/objects/function.hpp"

#include "hammer/vm/context.ipp"
#include "hammer/vm/objects/function.ipp"

namespace hammer::vm {

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

String FunctionTemplate::name() const noexcept {
    return access_heap<Data>()->name;
}

Module FunctionTemplate::module() const noexcept {
    return access_heap<Data>()->module;
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

ClosureContext
ClosureContext::make(Context& ctx, size_t size, Handle<ClosureContext> parent) {
    HAMMER_ASSERT(size > 0, "0 sized closure context is useless.");

    size_t total_size = variable_allocation<Data, Value>(size);
    Data* data = ctx.heap().create_varsize<Data>(
        total_size, parent, ctx.get_undefined(), size);
    return ClosureContext(from_heap(data));
}

ClosureContext ClosureContext::parent() const noexcept {
    return access_heap<Data>()->parent;
}

const Value* ClosureContext::data() const noexcept {
    return access_heap<Data>()->values;
}

size_t ClosureContext::size() const noexcept {
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

FunctionTemplate Function::tmpl() const noexcept {
    return access_heap<Data>()->tmpl;
}

ClosureContext Function::closure() const noexcept {
    return access_heap<Data>()->closure;
}

NativeFunction NativeFunction::make(
    Context& ctx, Handle<String> name, u32 min_params, SyncFunction function) {
    Data* data = ctx.heap().create<Data>();
    data->name = name;
    data->min_params = min_params;
    data->func = new SyncFunction(
        std::move(function)); // TODO use allocator from ctx
    return NativeFunction(from_heap(data));
}

NativeFunction NativeFunction::make_method(
    Context& ctx, Handle<String> name, u32 min_params, SyncFunction function) {
    HAMMER_CHECK(
        min_params > 0, "Methods must take at least one argument (`this`).");

    NativeFunction fn = make(ctx, name, min_params, function);
    fn.access_heap()->method = true;
    return fn;
}

String NativeFunction::name() const {
    return access_heap()->name;
}

u32 NativeFunction::min_params() const {
    return access_heap()->min_params;
}

const NativeFunction::SyncFunction& NativeFunction::function() const {
    HAMMER_CHECK(access_heap()->func != nullptr,
        "Native function was already finalized.");
    return *access_heap()->func;
}

bool NativeFunction::method() const {
    return access_heap()->method;
}

void NativeFunction::finalize() {
    Data* data = access_heap();
    HAMMER_CHECK(access_heap()->func != nullptr,
        "Native function was already finalized.");

    delete data->func;
    data->func = nullptr;
}

} // namespace hammer::vm
