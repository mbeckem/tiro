#include "vm/objects/buffer.hpp"

#include "vm/context.hpp"
#include "vm/math.hpp"

namespace tiro::vm {

Buffer Buffer::make(Context& ctx, size_t size, Uninitialized) {
    return make_impl(ctx, size, [&](Span<byte>) {});
}

Buffer Buffer::make(Context& ctx, size_t size, byte default_value) {
    return make_impl(ctx, size, [&](Span<byte> bytes) {
        TIRO_DEBUG_ASSERT(bytes.size() == size, "Unexpected buffer size.");
        std::uninitialized_fill_n(bytes.data(), bytes.size(), default_value);
    });
}

Buffer Buffer::make(Context& ctx, Span<const byte> content, size_t total_size, byte default_value) {
    TIRO_DEBUG_ASSERT(total_size >= content.size(), "Invalid size of initial content.");
    return make_impl(ctx, total_size, [&](Span<byte> bytes) {
        TIRO_DEBUG_ASSERT(bytes.size() == total_size, "Unexpected buffer size.");

        std::memcpy(bytes.data(), content.data(), content.size());
        std::uninitialized_fill_n(
            bytes.data() + content.size(), total_size - content.size(), default_value);
    });
}

byte Buffer::get(size_t index) const {
    TIRO_DEBUG_ASSERT(index < size(), "Buffer index out of bounds.");
    return *layout()->buffer_item(index);
}

void Buffer::set(size_t index, byte value) {
    TIRO_DEBUG_ASSERT(index < size(), "Buffer index out of bounds.");
    *layout()->buffer_item(index) = value;
}

size_t Buffer::size() const {
    return layout()->buffer_capacity();
}

byte* Buffer::data() const {
    return layout()->buffer_begin();
}

template<typename Init>
Buffer Buffer::make_impl(Context& ctx, size_t total_size, Init&& init) {
    auto type = ctx.types().internal_type<Buffer>();
    size_t allocation_size = LayoutTraits<Layout>::dynamic_size(total_size);
    Layout* data = ctx.heap().create_varsize<Layout>(
        allocation_size, type, BufferInit(total_size, init));
    return Buffer(Value::from_heap(data));
}

static constexpr MethodDesc buffer_methods[] = {
    {
        "size"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto buffer = check_instance<Buffer>(frame);
            i64 size = static_cast<i64>(buffer->size());
            frame.result(frame.ctx().get_integer(size));
        },
    },
};

constexpr TypeDesc buffer_type_desc{"Buffer"sv, buffer_methods};

} // namespace tiro::vm
