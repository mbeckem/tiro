#ifndef TIRO_CORE_DYNAMIC_BITSET_HPP
#define TIRO_CORE_DYNAMIC_BITSET_HPP

#include "tiro/core/defs.hpp"

#include <algorithm>
#include <vector>

namespace tiro {

/// A resizable bitset.
/// TODO: Placeholder impl based on std::vector.
/// TODO: Small buffer impl (-> small vec)
/// TODO: Tests
class DynamicBitset final {
public:
    using index_type = size_t;

    static constexpr index_type npos = index_type(-1);

    explicit DynamicBitset(index_type size)
        : bits_(size, false) {
        TIRO_DEBUG_ASSERT(size != npos, "Requested bitset size is too large.");
    }

    /// Returns the index of the first set bit (starting the search at index `first`).
    /// Returns npos if no set bit could be found.
    // TODO: Placeholder. implement in terms of hardware instruction.
    size_t first_set(size_t first = 0) const {
        TIRO_DEBUG_ASSERT(first <= size(), "Index out of bounds.");
        auto pos = std::find(bits_.begin() + first, bits_.end(), true);
        return pos == bits_.end() ? npos : pos - bits_.begin();
    }

    /// Returns the index of the first set bit (starting the search at index `first`).
    /// Returns npos if no set bit could be found.
    // TODO: Placeholder. implement in terms of hardware instruction.
    size_t first_unset(size_t first = 0) const {
        TIRO_DEBUG_ASSERT(first <= size(), "Index out of bounds.");
        auto pos = std::find(bits_.begin() + first, bits_.end(), false);
        return pos == bits_.end() ? npos : pos - bits_.begin();
    }

    /// Returns true if the bit at `index` is set.
    bool test(index_type index) const {
        TIRO_DEBUG_ASSERT(index < size(), "Index out of bounds.");
        return bits_[index];
    }

    /// Sets all bits to 0.
    void clear() { bits_.assign(bits_.size(), false); }

    /// Sets the bit at `index` to 0.
    void clear(index_type index) { set(index, false); }

    /// Sets the bit at `index` to the given value.
    void set(index_type index, bool value = true) {
        TIRO_DEBUG_ASSERT(index < size(), "Index out of bounds.");
        bits_[index] = value;
    }

    /// Resizes the set to the given new size. Additional elements (if any)
    /// will be initialized with `value`.
    void resize(index_type new_size, bool value = false) {
        TIRO_DEBUG_ASSERT(
            new_size != npos, "Requested bitset size is too large.");
        bits_.resize(new_size, value);
    }

    /// Resizes the set to the given new size if `new_size > size()`.
    void grow(index_type new_size, bool value = false) {
        if (new_size > size()) {
            resize(new_size, value);
        }
    }

    /// Returns the number of bits in the set.
    index_type size() const { return bits_.size(); }

private:
    std::vector<bool> bits_;
};

} // namespace tiro

#endif // TIRO_CORE_DYNAMIC_BITSET_HPP
