#include "hammer/vm/objects/strings.hpp"

#include "hammer/vm/context.hpp"
#include "hammer/vm/hash.hpp"

#include "hammer/vm/objects/arrays.ipp"
#include "hammer/vm/objects/strings.ipp"

namespace hammer::vm {

// TODO merge with other places.
static size_t next_exponential_capacity(size_t required) {
    static constexpr size_t max_pow = max_pow2<size_t>();
    if (required > max_pow) {
        return std::numeric_limits<size_t>::max();
    }
    return ceil_pow2(required);
}

String String::make(Context& ctx, std::string_view str) {
    return make_impl(ctx, str.size(),
        [&](char* storage) { std::memcpy(storage, str.data(), str.size()); });
}

String String::make(Context& ctx, Handle<StringBuilder> builder) {
    return make_impl(ctx, builder->size(), [&](char* storage) {
        std::memcpy(storage, builder->data(), builder->size());
    });
}

const char* String::data() const {
    return access_heap()->data;
}

size_t String::size() const {
    return access_heap()->size;
}

size_t String::hash() const {
    // TODO not thread safe
    size_t& hash = access_heap()->hash;
    size_t saved_flags = hash & ~hash_mask;
    if ((hash & hash_mask) == 0) {
        hash = byte_hash({reinterpret_cast<const byte*>(data()), size()});
        hash = hash == 0 ? 1 : hash;
        hash = (hash & hash_mask) | saved_flags;
    }
    return hash & hash_mask;
}

bool String::interned() const {
    return access_heap()->hash & interned_flag;
}

void String::interned(bool is_interned) {
    size_t& hash = access_heap()->hash;
    if (is_interned) {
        hash |= interned_flag;
    } else {
        hash &= ~interned_flag;
    }
}

bool String::equal(String other) const {
    HAMMER_ASSERT(!other.is_null(), "The other string must not be null.");

    if (interned() && other.interned())
        return same(other);
    return view() == other.view();
}

template<typename Init>
String String::make_impl(Context& ctx, size_t size, Init&& init) {
    const size_t allocation_size = variable_allocation<Data, char>(size);
    Data* data = ctx.heap().create_varsize<Data>(allocation_size, size);
    init(data->data);
    return String(from_heap(data));
}

StringBuilder StringBuilder::make(Context& ctx) {
    Data* data = ctx.heap().create<Data>();
    return StringBuilder(from_heap(data));
}

StringBuilder StringBuilder::make(Context& ctx, size_t initial_capacity) {
    initial_capacity = next_capacity(initial_capacity);
    Root<Buffer> buffer(ctx, Buffer::make(ctx, initial_capacity, 0));

    Data* data = ctx.heap().create<Data>();
    data->buffer = buffer.get();
    return StringBuilder(from_heap(data));
}

const char* StringBuilder::data() const {
    Data* d = access_heap();
    HAMMER_ASSERT(d->size == 0 || (d->buffer && d->buffer.size() >= d->size),
        "Invalid buffer, must be large enough if size is not 0.");
    return d->buffer ? reinterpret_cast<const char*>(d->buffer.data())
                     : nullptr;
}

size_t StringBuilder::size() const {
    Data* d = access_heap();
    return d->size;
}

size_t StringBuilder::capacity() const {
    Data* d = access_heap();
    return capacity(d);
}

void StringBuilder::append(Context& ctx, std::string_view str) {
    if (str.empty())
        return;

    Data* d = access_heap();
    reserve_free(d, ctx, str.size());
    append_impl(d, Span<const char>(str).as_bytes());
}

void StringBuilder::append(Context& ctx, Handle<String> str) {
    if (str->size() == 0)
        return;

    Data* d = access_heap();
    reserve_free(d, ctx, str->size());
    append_impl(d, Span<const char>(str->view()).as_bytes());
}

void StringBuilder::append(Context& ctx, Handle<StringBuilder> builder) {
    if (builder->size() == 0)
        return;

    Data* d = access_heap();
    reserve_free(d, ctx, builder->size());
    append_impl(d, Span<const char>(builder->view()).as_bytes());
}

std::string_view StringBuilder::view() const {
    return std::string_view(data(), size());
}

void StringBuilder::clear() {
    Data* data = access_heap();
    data->size = 0;
}

String StringBuilder::make_string(Context& ctx) {
    return String::make(ctx, Handle<StringBuilder>::from_slot(this));
}

byte* StringBuilder::reserve_free(Data* d, Context& ctx, size_t n) {
    size_t required = d->size;
    if (n == 0)
        return nullptr;

    if (HAMMER_UNLIKELY(!checked_add(required, n))) {
        // TODO exceptions
        HAMMER_ERROR("String too large.");
    }

    if (required > capacity(d)) {
        size_t new_capacity = next_capacity(required);
        if (d->buffer) {
            d->buffer = Buffer::make(ctx, d->buffer.values(), new_capacity, 0);
        } else {
            d->buffer = Buffer::make(ctx, new_capacity, 0);
        }
    }

    HAMMER_ASSERT(free(d) >= n, "Must have reserved enough capacity.");
    return d->buffer.data() + d->size;
}

void StringBuilder::append_impl(Data* d, Span<const byte> data) {
    HAMMER_ASSERT(free(d) >= data.size(), "Not enough free capacity.");

    std::memcpy(d->buffer.data() + d->size, data.data(), data.size());
    d->size += data.size();
}

size_t StringBuilder::free(Data* d) const {
    HAMMER_ASSERT(d->size <= capacity(d), "Cannot be more than full.");
    return capacity(d) - d->size;
}

size_t StringBuilder::capacity(Data* d) const {
    return d->buffer ? d->buffer.size() : 0;
}

size_t StringBuilder::next_capacity(size_t required) {
    return required <= 64 ? 64 : next_exponential_capacity(required);
}

} // namespace hammer::vm
