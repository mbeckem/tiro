#ifndef TIRO_VM_OBJECTS_STRING_HPP
#define TIRO_VM_OBJECTS_STRING_HPP

#include "common/adt/span.hpp"
#include "common/math.hpp"
#include "vm/object_support/fwd.hpp"
#include "vm/object_support/layout.hpp"
#include "vm/objects/value.hpp"

#include <optional>

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
    static String make(Context& ctx, Handle<StringSlice> slice);

    /// Formats the given message as a new string.
    /// Uses libfmt syntax.
    /// \warning arguments must stay stable in memory.
    template<typename... Args>
    static inline String format(Context& ctx, std::string_view fmt, const Args&... args) {
        return vformat(ctx, fmt, fmt::make_format_args(args...));
    }

    static String vformat(Context& ctx, std::string_view format, fmt::format_args args);

    /// This flag is set in the hash field if the string was interned.
    static constexpr size_t interned_flag = max_pow2<size_t>();

    /// Part of the hash field that represents the actual hash value.
    static constexpr size_t hash_mask = ~interned_flag;

    explicit String(Value v)
        : HeapValue(v, DebugCheck<String>()) {}

    /// Points to the beginning of the string string (invalidated by moves).
    const char* data();

    /// Returns the size of the string (in bytes).
    size_t size();

    /// Returns a string view over the string (invalidated by moves).
    std::string_view view() { return {data(), size()}; }

    /// Returns the hash value for this strings's content.
    size_t hash();

    /// Marks whether this string has been interned. Interned strings can be compared
    /// by comparing their addresses.
    bool interned();
    void interned(bool is_interned);

    /// Returns true if the other value is equal to *this. Supports Strings and StringSlices.
    bool equal(Value other);

    /// Returns a slice over the first `size` bytes.
    StringSlice slice_first(Context& ctx, size_t size);

    /// Returns a slice over the last `size` bytes.
    StringSlice slice_last(Context& ctx, size_t size);

    /// Returns a slice of `size` bytes, starting at the given offset.
    StringSlice slice(Context& ctx, size_t offset, size_t size);

    Layout* layout() const { return access_heap<Layout>(); }

private:
    template<typename Init>
    static String make_impl(Context& ctx, size_t total_size, Init&& init);
};

class StringSlice final : public HeapValue {
private:
    enum Slots {
        StringSlot,
        SlotCount_,
    };

    struct Payload {
        size_t offset;
        size_t size;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    static StringSlice make(Context& ctx, Handle<String> str, size_t offset, size_t size);
    static StringSlice make(Context& ctx, Handle<StringSlice> slice, size_t offset, size_t size);

    explicit StringSlice(Value v)
        : HeapValue(v, DebugCheck<StringSlice>()) {}

    /// Returns the original string that is referenced by this slice.
    String original() { return get_string(); }

    /// Returns the offset where this slice starts in the original string.
    size_t offset();

    /// Points to the beginning of the string slice (invalidated by moves).
    const char* data();

    /// Returns the size of the slice (in bytes).
    size_t size();

    /// Returns a string view over the slice (invalidated by moves).
    std::string_view view() { return {data(), size()}; }

    /// Returns the hash value for this slice's content. Compatible with String hash values.
    size_t hash();

    /// Returns true if the other value is equal to *this. Supports Strings and StringSlices.
    bool equal(Value other);

    /// Returns a slice over the first `size` bytes.
    StringSlice slice_first(Context& ctx, size_t size);

    /// Returns a slice over the last `size` bytes.
    StringSlice slice_last(Context& ctx, size_t size);

    /// Returns a slice of `size` bytes, starting at the given offset.
    StringSlice slice(Context& ctx, size_t offset, size_t size);

    /// Constructs a new string instance with the same content as this slice.
    String to_string(Context& ctx);

    Layout* layout() const { return access_heap<Layout>(); }

private:
    String get_string();
};

/// Iterates over an string or a string slice.
class StringIterator final : public HeapValue {
private:
    enum Slots {
        StringSlot,
        SlotCount_,
    };

    struct Payload {
        size_t index;
        size_t end;
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>, StaticPayloadPiece<Payload>>;

    static StringIterator make(Context& ctx, Handle<String> string);
    static StringIterator make(Context& ctx, Handle<StringSlice> slice);

    explicit StringIterator(Value v)
        : HeapValue(v, DebugCheck<StringIterator>()) {}

    // FIXME: Horrendous performance (one allocation for each character in a string).
    //        Chars can be optimized in the same way as small integers by packing them into the pointer instead!
    // FIXME: Chars should be unicode glyphs instead of bytes!
    std::optional<Value> next(Context& ctx);

    Layout* layout() const { return access_heap<Layout>(); }
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

    /// Append the given string slice to this one.
    void append(Context& ctx, Handle<StringSlice> slice);

    /// Formats the given message and appends in to the builder.
    /// Uses libfmt syntax.
    /// \warning arguments must stay stable in memory.
    template<typename... Args>
    inline void format(Context& ctx, std::string_view fmt, const Args&... args);
    void vformat(Context& ctx, std::string_view format, fmt::format_args args);

    /// Create a new string with the current content.
    String to_string(Context& ctx);

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

    Nullable<Buffer> get_buffer(Layout* data);
    void set_buffer(Layout* data, Nullable<Buffer> buffer);

    static size_t next_capacity(size_t required);
};

template<typename... Args>
void StringBuilder::format(Context& ctx, std::string_view format, const Args&... args) {
    return vformat(ctx, format, fmt::make_format_args(args...));
}

extern const TypeDesc string_type_desc;
extern const TypeDesc string_slice_type_desc;
extern const TypeDesc string_builder_type_desc;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_STRING_HPP
