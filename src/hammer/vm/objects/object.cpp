#include "hammer/vm/objects/object.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/context.ipp"
#include "hammer/vm/objects/object.ipp"

#include <new>

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

Float Float::make(Context& ctx, f64 value) {
    Data* data = ctx.heap().create<Data>(value);
    return Float(from_heap(data));
}

f64 Float::value() const noexcept {
    return access_heap<Data>()->value;
}

SpecialValue SpecialValue::make(Context& ctx, std::string_view name) {
    // TODO use String as argument type instead for interning.
    Root<String> name_str(ctx, String::make(ctx, name));

    Data* data = ctx.heap().create<Data>(name_str.get());
    return SpecialValue(from_heap(data));
}

std::string_view SpecialValue::name() const {
    return access_heap()->name.view();
}

Symbol Symbol::make(Context& ctx, Handle<String> name) {
    HAMMER_CHECK(!name->is_null(), "The symbol name must be a valid string.");

    Data* data = ctx.heap().create<Data>(name.get());
    return Symbol(from_heap(data));
}

String Symbol::name() const {
    return access_heap()->name;
}

bool Symbol::equal(Symbol other) const {
    return same(other);
}

Module Module::make(Context& ctx, Handle<String> name, Handle<Tuple> members,
    Handle<HashTable> exported) {
    Data* data = ctx.heap().create<Data>(name, members, exported);
    return Module(from_heap(data));
}

String Module::name() const noexcept {
    return access_heap<Data>()->name;
}

Tuple Module::members() const noexcept {
    return access_heap<Data>()->members;
}

HashTable Module::exported() const {
    return access_heap<Data>()->exported;
}

Tuple Tuple::make(Context& ctx, size_t size) {
    return make_impl(ctx, size, [&](Data* d) {
        std::uninitialized_fill_n(d->values, size, Value::null());
    });
}

Tuple Tuple::make(Context& ctx, Span<const Value> values) {
    return make_impl(ctx, values.size(), [&](Data* d) {
        std::uninitialized_copy_n(values.data(), values.size(), d->values);
    });
}

Tuple Tuple::make(Context& ctx, Span<const Value> values, size_t total_values) {
    HAMMER_ASSERT(total_values >= values.size(),
        "Tuple::make(): invalid total_size, must be >= values.size().");

    const size_t copy = values.size();
    const size_t fill = total_values - copy;
    return make_impl(ctx, total_values, [&](Data* d) {
        Value* pos = std::uninitialized_copy_n(values.data(), copy, d->values);
        std::uninitialized_fill_n(pos, fill, Value::null());
    });
}

Tuple Tuple::make(Context& ctx, std::initializer_list<Handle<Value>> values) {
    return make_impl(ctx, values.size(), [&](Data* d) {
        const Handle<Value>* pos = values.begin();
        const Handle<Value>* end = values.end();
        Value* dst = d->values;
        for (; pos != end; ++pos, ++dst) {
            new (dst) Value(*pos);
        }
    });
}

const Value* Tuple::data() const {
    return access_heap()->values;
}

size_t Tuple::size() const {
    return access_heap()->size;
}

Value Tuple::get(size_t index) const {
    // TODO this should be a language level exception
    HAMMER_CHECK(index < size(), "Tuple::get(): index out of bounds.");
    return access_heap()->values[index];
}

void Tuple::set(WriteBarrier, size_t index, Value value) const {
    // TODO Exception
    HAMMER_CHECK(index < size(), "Tuple::set(): index out of bounds.");
    access_heap()->values[index] = value;
}

template<typename Init>
Tuple Tuple::make_impl(Context& ctx, size_t total_size, Init&& init) {
    const size_t allocation_size = variable_allocation<Data, Value>(total_size);
    Data* data = ctx.heap().create_varsize<Data>(
        allocation_size, total_size, std::forward<Init>(init));
    return Tuple(from_heap(data));
}

} // namespace hammer::vm