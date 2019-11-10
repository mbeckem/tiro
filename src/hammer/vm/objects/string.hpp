#ifndef HAMMER_VM_OBJECTS_STRING_HPP
#define HAMMER_VM_OBJECTS_STRING_HPP

#include "hammer/vm/objects/value.hpp"

namespace hammer::vm {

/**
 * Represents a string.
 * 
 * TODO: Unicode stuff.
 */
class String final : public Value {
public:
    static String make(Context& ctx, std::string_view str);

    // This flag is set in the hash field if the string was interned.
    static constexpr size_t interned_flag = max_pow2<size_t>();

    // Part of the hash field that represents the actual hash value.
    static constexpr size_t hash_mask = ~interned_flag;

    String() = default;

    explicit String(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<String>(), "Value is not a string.");
    }

    std::string_view view() const noexcept { return {data(), size()}; }

    const char* data() const noexcept;
    size_t size() const noexcept;

    size_t hash() const noexcept;

    bool interned() const;
    void interned(bool is_interned);

    bool equal(String other) const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&&);

private:
    struct Data;

    inline Data* access_heap() const;
};

class StringBuilder final : public Value {
public:
    static StringBuilder make(Context& ctx);
    static StringBuilder make(Context& ctx, size_t initial_capacity);

    StringBuilder() = default;

    explicit StringBuilder(Value v)
        : Value(v) {
        HAMMER_ASSERT(v.is<StringBuilder>(), "Value is not a string builder.");
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
    void append(Context& ctx, std::string_view str);

    /// Formats the given message and appends in to the builder.
    /// Uses libfmt syntax.
    template<typename... Args>
    inline void format(Context& ctx, std::string_view fmt, Args&&... args);

    /// Create a new string with the current content.
    String make_string(Context& ctx) const;

    inline size_t object_size() const noexcept;

    template<typename W>
    inline void walk(W&& w);

private:
    struct Data;

    // Makes sure that at least n bytes can be appended. Invalidates
    // other pointers to the internal storage.
    u8* reserve_free(Data* d, Context& ctx, size_t n);

    // Number of available bytes.
    size_t free(Data* d) const;

    // Number of allocated bytes.
    size_t capacity(Data* d) const;

    inline Data* access_heap() const;

    static size_t next_capacity(size_t required);
};

} // namespace hammer::vm

#endif // HAMMER_VM_OBJECTS_STRING_HPP
