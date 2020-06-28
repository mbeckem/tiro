#ifndef TIRO_VM_OBJECTS_STRING_HPP
#define TIRO_VM_OBJECTS_STRING_HPP

#include "common/math.hpp"
#include "common/span.hpp"
#include "vm/objects/layout.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

/// Represents a string.
///
/// TODO: Unicode stuff.
class String final : public HeapValue {
private:
    struct Payload {
        size_t hash = 0; // Lazy, one bit used for interned flag.
    };

public:
    using Layout = BufferLayout<char, alignof(char), StaticPayloadPiece<Payload>>;

    static String make(Context& ctx, std::string_view str);
    static String make(Context& ctx, Handle<StringBuilder> builder);

    // This flag is set in the hash field if the string was interned.
    static constexpr size_t interned_flag = max_pow2<size_t>();

    // Part of the hash field that represents the actual hash value.
    static constexpr size_t hash_mask = ~interned_flag;

    String() = default;

    explicit String(Value v)
        : HeapValue(v, DebugCheck<String>()) {}

    std::string_view view() { return {data(), size()}; }

    const char* data();
    size_t size();

    size_t hash();

    bool interned();
    void interned(bool is_interned);

    bool equal(String other);

    Layout* layout() const { return access_heap<Layout>(); }

private:
    template<typename Init>
    static String make_impl(Context& ctx, size_t total_size, Init&& init);
};

/// A resizable buffer that cat be used to assemble a string.
class StringBuilder final : public HeapValue {
private:
    enum Slots {
        BufferSlot,
        SlotCount_,
    };

    struct Payload {
        size_t size = 0;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    static StringBuilder make(Context& ctx);
    static StringBuilder make(Context& ctx, size_t initial_capacity);

    StringBuilder() = default;

    explicit StringBuilder(Value v)
        : HeapValue(v, DebugCheck<StringBuilder>()) {}

    /// Points to the internal character storage.
    /// Invalidated by append operations!
    const char* data();

    /// Number of bytes accessible from data().
    size_t size();

    /// Total capacity (in bytes).
    size_t capacity();

    /// Returns a string view over the current content.
    /// Invalidated by append operations!
    std::string_view view();

    /// Resets the content of this builder (but does not release any memory).
    void clear();

    /// Append the given string to the builder.
    /// \warning `str` must stay stable in memory.
    void append(Context& ctx, std::string_view str);

    /// Append the given string to the builder.
    void append(Context& ctx, Handle<String> str);

    /// Append the content of the given string builder to this one.
    void append(Context& ctx, Handle<StringBuilder> builder);

    /// Formats the given message and appends in to the builder.
    /// Uses libfmt syntax.
    /// \warning arguments must stay stable in memory.
    template<typename... Args>
    inline void format(Context& ctx, std::string_view fmt, Args&&... args);

    /// Create a new string with the current content.
    String make_string(Context& ctx);

    Layout* layout() const { return access_heap<Layout>(); }

private:
    // Makes sure that at least n bytes can be appended. Invalidates
    // other pointers to the internal storage.
    byte* reserve_free(Layout* data, Context& ctx, size_t n);

    // Appends the given bytes (capacity must have been alloacted!)
    void append_impl(Layout* data, Span<const byte> bytes);

    // Number of available bytes.
    size_t free(Layout* data);

    // Number of allocated bytes.
    size_t capacity(Layout* data);

    Buffer get_buffer(Layout* data);
    void set_buffer(Layout* data, Buffer buffer);

    static size_t next_capacity(size_t required);
};

template<typename... Args>
void StringBuilder::format(Context& ctx, std::string_view fmt, Args&&... args) {
    Layout* data = layout();

    const size_t size = fmt::formatted_size(fmt, args...);
    if (size == 0)
        return;

    u8* buffer = reserve_free(data, ctx, size);
    fmt::format_to(buffer, fmt, args...);
    data->static_payload()->size += size;
}

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_STRING_HPP
