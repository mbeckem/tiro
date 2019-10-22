#include "hammer/vm/object.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/context.ipp"
#include "hammer/vm/object.ipp"

namespace hammer::vm {

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

void ClosureContext::set(WriteBarrier, size_t index, Value value) const {
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

Module
Module::make(Context& ctx, Handle<String> name, Handle<FixedArray> members) {
    Data* data = ctx.heap().create<Data>(name, members);
    return Module(from_heap(data));
}

String Module::name() const noexcept {
    return access_heap<Data>()->name;
}

FixedArray Module::members() const noexcept {
    return access_heap<Data>()->members;
}

FixedArray FixedArray::make(Context& ctx, size_t size) {
    size_t total_size = variable_allocation<Data, Value>(size);
    Data* data = ctx.heap().create_varsize<Data>(total_size, size);
    return FixedArray(from_heap(data));
}

FixedArray FixedArray::make(Context& ctx, Span<const Value> values) {
    size_t total_size = variable_allocation<Data, Value>(values.size());
    Data* data = ctx.heap().create_varsize<Data>(total_size, values);
    return FixedArray(from_heap(data));
}

FixedArray
FixedArray::make(Context& ctx, Span<const Value> values, size_t total_values) {
    HAMMER_ASSERT(
        total_values >= values.size(), "total_values is not large enough.");

    size_t total_size = variable_allocation<Data, Value>(total_values);
    Data* data = ctx.heap().create_varsize<Data>(
        total_size, values, total_values);
    return FixedArray(from_heap(data));
}

const Value* FixedArray::data() const noexcept {
    return access_heap<Data>()->values;
}

size_t FixedArray::size() const noexcept {
    return access_heap<Data>()->size;
}

Value FixedArray::get(size_t index) const {
    // TODO Exception
    HAMMER_CHECK(index < size(), "FixedArray::get(): index out of bounds.");
    return access_heap<Data>()->values[index];
}

void FixedArray::set(WriteBarrier, size_t index, Value value) const {
    // TODO Exception
    HAMMER_CHECK(index < size(), "FixedArray::set(): index out of bounds.");
    access_heap<Data>()->values[index] = value;
}

Array Array::make(Context& ctx, size_t initial_capacity) {
    initial_capacity = next_capacity(initial_capacity);

    Root<FixedArray> storage(ctx);
    if (initial_capacity > 0) {
        storage.set(FixedArray::make(ctx, initial_capacity));
    }

    Data* data = ctx.heap().create<Data>();
    data->storage = storage;
    return Array(from_heap(data));
}

Array Array::make(Context& ctx, Span<const Value> initial_content) {
    if (initial_content.empty())
        return make(ctx, 0);

    Root<FixedArray> storage(ctx, FixedArray::make(ctx, initial_content));

    Data* data = ctx.heap().create<Data>();
    data->storage = storage;
    data->size = initial_content.size();
    return Array(from_heap(data));
}

const Value* Array::data() const noexcept {
    Data* d = access_heap<Data>();
    return d->storage.is_null() ? nullptr : d->storage.data();
}

size_t Array::size() const noexcept {
    return access_heap<Data>()->size;
}

size_t Array::capacity() const noexcept {
    Data* d = access_heap<Data>();
    return d->storage.is_null() ? 0 : d->storage.size();
}

Value Array::get(size_t index) const {
    // TODO Exception
    HAMMER_CHECK(index < size(), "Array::get(): index out of bounds.");
    HAMMER_ASSERT(index < access_heap<Data>()->storage.size(),
        "Storage capacity must be sufficient.");
    return access_heap<Data>()->storage.get(index);
}

void Array::set(Context& ctx, size_t index, Value value) const {
    // TODO Exception
    HAMMER_CHECK(index < size(), "Array::set(): index out of bounds.");
    HAMMER_ASSERT(index < access_heap<Data>()->storage.size(),
        "Storage capacity must be sufficient.");
    Data* d = access_heap<Data>();
    HAMMER_WRITE_INDEX(ctx, d->storage, index, value);
}

void Array::append(Context& ctx, Handle<Value> value) const {
    Data* const d = access_heap<Data>();

    size_t index = d->size;
    if (index >= capacity()) {
        size_t new_size = index;
        if (HAMMER_UNLIKELY(!checked_add(new_size, size_t(1))))
            HAMMER_ERROR("Array size too large."); // FIXME exception

        new_size = next_capacity(new_size);
        d->storage = FixedArray::make(ctx, d->storage.values(), new_size);
    }

    HAMMER_ASSERT(index < capacity(), "Storage was not made large enough.");
    HAMMER_WRITE_INDEX(ctx, d->storage, index, value);
}

size_t Array::next_capacity(size_t required) {
    static constexpr size_t max_pow = max_pow2<size_t>();
    if (HAMMER_UNLIKELY(required > max_pow)) {
        // TODO: We can if not bound by power of 2
        HAMMER_ERROR("Cannot allocate arrays this large.");
    }

    if (required == 0)
        return 0;
    if (required <= 8)
        return 8;
    return ceil_pow2(required);
}

} // namespace hammer::vm
