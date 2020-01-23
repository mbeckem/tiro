#ifndef TIRO_OBJECTS_STRINGS_HPP
#define TIRO_OBJECTS_STRINGS_HPP

#include "tiro/core/span.hpp"
#include "tiro/objects/value.hpp"

namespace tiro::vm {

/// Represents a string.
///
/// TODO: Unicode stuff.
class String final : public Value {
public:
    static String make(Context& ctx, std::string_view str);
    static String make(Context& ctx, Handle<StringBuilder> builder);

    // This flag is set in the hash field if the string was interned.
    static constexpr size_t interned_flag = max_pow2<size_t>();

    // Part of the hash field that represents the actual hash value.
    static constexpr size_t hash_mask = ~interned_flag;

    String() = default;

    explicit String(Value v)
        : Value(v) {
        TIRO_ASSERT(v.is<String>(), "Value is not a string.");
    }

    std::string_view view() const { return {data(), size()}; }

    const char* data() const;
    size_t size() const;

    size_t hash() const;

    bool interned() const;
    void interned(bool is_interned);

    bool equal(String other) const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;

    template<typename Init>
    static String make_impl(Context& ctx, size_t total_size, Init&& init);

    inline Data* access_heap() const;
};

/// A resizable buffer that cat be used to assemble a string.
class StringBuilder final : public Value {
public:
    static StringBuilder make(Context& ctx);
    static StringBuilder make(Context& ctx, size_t initial_capacity);

    StringBuilder() = default;

    explicit StringBuilder(Value v)
        : Value(v) {
        TIRO_ASSERT(v.is<StringBuilder>(), "Value is not a string builder.");
    }

    /// Points to the internal character storage.
    /// Invalidated by append operations!
    const char* data() const;

    /// Number of bytes accessible from data().
    size_t size() const;

    /// Total capacity (in bytes).
    size_t capacity() const;

    /// Returns a string view over the current content.
    /// Invalidated by append operations!
    std::string_view view() const;

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

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    // Makes sure that at least n bytes can be appended. Invalidates
    // other pointers to the internal storage.
    byte* reserve_free(Data* d, Context& ctx, size_t n);

    // Appends the given data (capacity must have been alloacted!)
    void append_impl(Data* d, Span<const byte> data);

    // Number of available bytes.
    size_t free(Data* d) const;

    // Number of allocated bytes.
    size_t capacity(Data* d) const;

    inline Data* access_heap() const;

    static size_t next_capacity(size_t required);
};

} // namespace tiro::vm

#endif // TIRO_OBJECTS_STRINGS_HPP
