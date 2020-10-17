#ifndef TIRO_COMMON_COMMON_TAGGED_PTR_HPP
#define TIRO_COMMON_COMMON_TAGGED_PTR_HPP

#include "common/assert.hpp"
#include "common/defs.hpp"

#include <climits>

namespace tiro {

/// A tagged pointer contains an aligned native pointer augmented with a few
/// used defined tag bits. By ensuring that the raw pointer value always contains
/// enough trailing zeroes at the end, tag bits can be inserted without destroying
/// the original pointer.
///
/// This class is used to store additional metadata with a pointer, for example a simple type tag.
/// The user of this class *must* make sure that all pointers are aligned correctly (see constant
/// `ptr_alignment`).
template<size_t TagBits>
class TaggedPtr final {
public:
    /// Number of bits in a pointer.
    static constexpr size_t total_bits = CHAR_BIT * sizeof(uintptr_t);

    /// Number of bits available for user data.
    static constexpr size_t tag_bits = TagBits;

    /// Number of bits used for the storage of the actual pointer.
    static constexpr size_t pointer_bits = total_bits - tag_bits;

    /// Minimum alignment required for all pointer values.
    static constexpr size_t pointer_alignment = 1 << tag_bits;

    /// When bulk setting the tag, the values must be smaller than this constant.
    static constexpr size_t max_tag_value = 1 << tag_bits;

    static_assert(tag_bits > 0, "TagBits must be at least 1.");
    static_assert(tag_bits < total_bits, "Not enough space left for a pointer value.");

private:
    static constexpr uintptr_t tag_mask = max_tag_value - 1;
    static constexpr uintptr_t ptr_mask = ~tag_mask;

public:
    /// Constructs a null pointer with a 0 tag.
    TaggedPtr() = default;

    /// Constructs a pointer with the given value and a 0 tag.
    /// \pre `ptr` must be aligned according to `pointer_alignment`.
    TaggedPtr(void* ptr)
        : TaggedPtr(ptr, 0) {}

    /// Constructs a pointer with the given value and tag.
    /// \pre `ptr` must be aligned according to `pointer_alignment`.
    /// \pre `tag < max_tag_value`.
    TaggedPtr(void* ptr, uintptr_t tag)
        : raw_(combine(ptr, tag)) {}

    /// Returns the current pointer value.
    void* pointer() const { return extract_ptr(raw_); }

    /// Sets the pointer value to `new_ptr`.
    /// \pre `new_ptr` must be aligned according to `pointer_alignment`.
    void pointer(void* new_ptr) { raw_ = combine(new_ptr, tag()); }

    /// Returns the current tag value.
    uintptr_t tag() const { return extract_tag(raw_); }

    /// Sets the tag value to `new_tag`.
    /// \pre `new_tag < max_tag_value`.
    void tag(uintptr_t new_tag) { raw_ = combine(pointer(), new_tag); }

    /// Returns the tag bit with the given index. The index is checked at compile time.
    template<size_t Index>
    bool tag_bit() const {
        static_assert(Index < tag_bits, "Tag bit index out of bounds.");
        return raw_ & (uintptr_t(1) << Index);
    }

    template<size_t Index>
    void tag_bit(bool set) {
        static_assert(Index < tag_bits, "Tag bit index out of bounds.");
        if (set) {
            raw_ |= (uintptr_t(1) << Index);
        } else {
            raw_ &= ~(uintptr_t(1) << Index);
        }
    }

private:
    static uintptr_t combine(void* ptr, uintptr_t tag) {
        uintptr_t raw_ptr = reinterpret_cast<uintptr_t>(ptr);
        TIRO_DEBUG_ASSERT((raw_ptr & tag_mask) == 0, "Pointer value is not aligned correctly.");
        TIRO_DEBUG_ASSERT((tag & ptr_mask) == 0, "Tag value is too large.");
        return raw_ptr | tag;
    }

    static void* extract_ptr(uintptr_t combined) {
        return reinterpret_cast<void*>(combined & ptr_mask);
    }

    static uintptr_t extract_tag(uintptr_t combined) { return combined & tag_mask; }

private:
    uintptr_t raw_ = 0;
};

} // namespace tiro

#endif // TIRO_COMMON_COMMON_TAGGED_PTR_HPP
