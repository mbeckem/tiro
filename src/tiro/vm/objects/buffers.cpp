#include "tiro/vm/objects/buffers.hpp"

#include "tiro/vm/context.hpp"

#include "tiro/vm/objects/buffers.ipp"

namespace tiro::vm {

template<typename Init>
Buffer Buffer::make_impl(Context& ctx, size_t total_size, Init&& init) {
    size_t allocation_size = variable_allocation<Data, byte>(total_size);
    Data* data = ctx.heap().create_varsize<Data>(allocation_size, total_size);
    init(data);
    return Buffer(Value::from_heap(data));
}

Buffer Buffer::make(Context& ctx, size_t size, Uninitialized) {
    return make_impl(ctx, size, [&](Data*) {});
}

Buffer Buffer::make(Context& ctx, size_t size, byte default_value) {
    return make_impl(ctx, size, [&](Data* data) {
        std::uninitialized_fill_n(data->values, size, default_value);
    });
}

Buffer Buffer::make(Context& ctx, Span<const byte> content, size_t total_size,
    byte default_value) {
    TIRO_ASSERT(
        total_size >= content.size(), "Invalid size of initial content.");
    return make_impl(ctx, total_size, [&](Data* data) {
        std::memcpy(data->values, content.data(), content.size());
        std::uninitialized_fill_n(data->values + content.size(),
            total_size - content.size(), default_value);
    });
}

byte Buffer::get(size_t index) const {
    TIRO_ASSERT(index < size(), "Buffer index out of bounds.");
    return access_heap()->values[index];
}

void Buffer::set(size_t index, byte value) const {
    TIRO_ASSERT(index < size(), "Buffer index out of bounds.");
    access_heap()->values[index] = value;
}

size_t Buffer::size() const {
    return access_heap()->size;
}

byte* Buffer::data() const {
    return access_heap()->values;
}

} // namespace tiro::vm
