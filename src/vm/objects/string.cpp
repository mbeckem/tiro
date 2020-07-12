#include "vm/objects/string.hpp"

#include "vm/context.hpp"
#include "vm/hash.hpp"
#include "vm/math.hpp"
#include "vm/objects/buffer.hpp"
#include "vm/objects/factory.hpp"
#include "vm/objects/native.hpp"

namespace tiro::vm {

static size_t next_exponential_capacity(size_t required) {
    static constexpr size_t max_pow = max_pow2<size_t>();
    if (required > max_pow) {
        return std::numeric_limits<size_t>::max();
    }
    return ceil_pow2(required);
}

String String::make(Context& ctx, std::string_view str) {
    return make_impl(ctx, str.size(),
        [&](Span<char> chars) { std::memcpy(chars.data(), str.data(), str.size()); });
}

String String::make(Context& ctx, Handle<StringBuilder> builder) {
    return make_impl(ctx, builder->size(),
        [&](Span<char> chars) { std::memcpy(chars.data(), builder->data(), builder->size()); });
}

const char* String::data() {
    return layout()->buffer_begin();
}

size_t String::size() {
    return layout()->buffer_capacity();
}

size_t String::hash() {
    // TODO not thread safe
    size_t& hash = layout()->static_payload()->hash;
    size_t saved_flags = hash & ~hash_mask;
    if ((hash & hash_mask) == 0) {
        hash = byte_hash({reinterpret_cast<const byte*>(data()), size()});
        hash = hash == 0 ? 1 : hash;
        hash = (hash & hash_mask) | saved_flags;
    }
    return hash & hash_mask;
}

bool String::interned() {
    return layout()->static_payload()->hash & interned_flag;
}

void String::interned(bool is_interned) {
    size_t& hash = layout()->static_payload()->hash;
    if (is_interned) {
        hash |= interned_flag;
    } else {
        hash &= ~interned_flag;
    }
}

bool String::equal(String other) {
    TIRO_DEBUG_ASSERT(!other.is_null(), "The other string must not be null.");

    if (interned() && other.interned())
        return same(other);

    return view() == other.view();
}

template<typename Init>
String String::make_impl(Context& ctx, size_t size, Init&& init) {
    Layout* data = create_object<String>(ctx, size, BufferInit(size, init), StaticPayloadInit());
    return String(from_heap(data));
}

StringBuilder StringBuilder::make(Context& ctx) {
    Layout* data = create_object<StringBuilder>(ctx, StaticSlotsInit(), StaticPayloadInit());
    return StringBuilder(from_heap(data));
}

StringBuilder StringBuilder::make(Context& ctx, size_t initial_capacity) {
    size_t adjusted_capacity = next_capacity(initial_capacity);

    Scope sc(ctx);
    Local buffer = sc.local(Buffer::make(ctx, adjusted_capacity, 0));

    Layout* data = create_object<StringBuilder>(ctx, StaticSlotsInit(), StaticPayloadInit());
    data->write_static_slot(BufferSlot, buffer.get());
    return StringBuilder(from_heap(data));
}

const char* StringBuilder::data() {
    Layout* data = layout();

    auto buffer = get_buffer(data);
    TIRO_DEBUG_ASSERT(size() == 0 || (buffer && buffer.value().size() >= size()),
        "Invalid buffer, must be large enough if size is not 0.");
    return buffer ? reinterpret_cast<const char*>(buffer.value().data()) : nullptr;
}

size_t StringBuilder::size() {
    Layout* data = layout();
    return data->static_payload()->size;
}

size_t StringBuilder::capacity() {
    Layout* data = layout();
    return capacity(data);
}

void StringBuilder::append(Context& ctx, std::string_view str) {
    if (str.empty())
        return;

    Layout* d = layout();
    reserve_free(d, ctx, str.size());
    append_impl(d, Span<const char>(str).as_bytes());
}

void StringBuilder::append(Context& ctx, Handle<String> str) {
    if (str->size() == 0)
        return;

    Layout* d = layout();
    reserve_free(d, ctx, str->size());
    append_impl(d, Span<const char>(str->view()).as_bytes());
}

void StringBuilder::append(Context& ctx, Handle<StringBuilder> builder) {
    if (builder->size() == 0)
        return;

    Layout* d = layout();
    reserve_free(d, ctx, builder->size());
    append_impl(d, Span<const char>(builder->view()).as_bytes());
}

std::string_view StringBuilder::view() {
    return std::string_view(data(), size());
}

void StringBuilder::clear() {
    Layout* data = layout();
    data->static_payload()->size = 0;
}

String StringBuilder::make_string(Context& ctx) {
    return String::make(ctx, Handle<StringBuilder>(this));
}

byte* StringBuilder::reserve_free(Layout* data, Context& ctx, size_t n) {
    size_t required = data->static_payload()->size;
    if (n == 0)
        return nullptr;

    if (TIRO_UNLIKELY(!checked_add(required, n))) {
        // TODO exceptions
        TIRO_ERROR("String too large.");
    }

    // Fast path: enough capacity.
    if (required <= capacity(data)) {
        auto buffer = get_buffer(data).value();
        return buffer.data() + size();
    }

    // Slow path: allocate new buffer
    Scope sc(ctx);
    Local old_buffer = sc.local(get_buffer(data));
    Local new_buffer = sc.local<Nullable<Buffer>>();

    size_t new_capacity = next_capacity(required);
    if (old_buffer->has_value()) {
        // FIXME unsafe when moving
        new_buffer = Buffer::make(ctx, old_buffer->value().values(), new_capacity, 0);
    } else {
        new_buffer = Buffer::make(ctx, new_capacity, 0);
    }

    set_buffer(data, *new_buffer);
    TIRO_DEBUG_ASSERT(free(data) >= n, "Must have reserved enough capacity.");
    return new_buffer.must_cast<Buffer>()->data() + size();
}

void StringBuilder::append_impl(Layout* data, Span<const byte> bytes) {
    TIRO_DEBUG_ASSERT(free(data) >= bytes.size(), "Not enough free capacity.");

    auto buffer = get_buffer(data).value();
    std::memcpy(buffer.data() + size(), bytes.data(), bytes.size());
    data->static_payload()->size += bytes.size();
}

size_t StringBuilder::free(Layout* data) {
    TIRO_DEBUG_ASSERT(size() <= capacity(data), "Cannot be more than full.");
    return capacity(data) - size();
}

size_t StringBuilder::capacity(Layout* data) {
    auto buffer = get_buffer(data);
    return buffer ? buffer.value().size() : 0;
}

Nullable<Buffer> StringBuilder::get_buffer(Layout* data) {
    return data->read_static_slot<Nullable<Buffer>>(BufferSlot);
}

void StringBuilder::set_buffer(Layout* data, Nullable<Buffer> buffer) {
    data->write_static_slot(BufferSlot, buffer);
}

size_t StringBuilder::next_capacity(size_t required) {
    return required <= 64 ? 64 : next_exponential_capacity(required);
}

static constexpr MethodDesc string_methods[] = {
    {
        "size"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto string = check_instance<StringBuilder>(frame);
            frame.result(string->make_string(frame.ctx()));
        },
    },
};

constexpr TypeDesc string_type_desc{"String"sv, string_methods};

static constexpr MethodDesc string_builder_methods[] = {
    {
        "append"sv,
        2,
        [](NativeFunctionFrame& frame) {
            auto builder = check_instance<StringBuilder>(frame);
            for (size_t i = 1; i < frame.arg_count(); ++i) {
                Handle<Value> arg = frame.arg(i);
                if (auto str = arg.try_cast<String>()) {
                    builder->append(frame.ctx(), str.handle());
                } else if (auto b = arg.try_cast<StringBuilder>()) {
                    builder->append(frame.ctx(), b.handle());
                } else {
                    TIRO_ERROR("Cannot append values of type {}.", to_string(arg->type()));
                }
            }
        },
        MethodDesc::Variadic,
    },
    {
        "append_byte"sv,
        2,
        [](NativeFunctionFrame& frame) {
            auto builder = check_instance<StringBuilder>(frame);
            Handle arg = frame.arg(1);

            byte b;
            if (auto i = try_extract_integer(*arg); i && *i >= 0 && *i <= 0xff) {
                b = *i;
            } else {
                TIRO_ERROR("Expected a byte argument (between 0 and 255).");
            }

            builder->append(frame.ctx(), std::string_view((char*) &b, 1));
        },
    },
    {
        "clear"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto builder = check_instance<StringBuilder>(frame);
            builder->clear();
        },
    },
    {
        "size"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto builder = check_instance<StringBuilder>(frame);
            size_t size = static_cast<size_t>(builder->size());
            frame.result(frame.ctx().get_integer(size));
        },
    },
    {
        "to_str"sv,
        1,
        [](NativeFunctionFrame& frame) {
            auto builder = check_instance<StringBuilder>(frame);
            frame.result(builder->make_string(frame.ctx()));
        },
    },
};

constexpr TypeDesc string_builder_type_desc{"StringBuilder"sv, string_builder_methods};

} // namespace tiro::vm
