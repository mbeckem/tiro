#ifndef TIRO_COMMON_ADT_BITSET_HPP
#define TIRO_COMMON_ADT_BITSET_HPP

#include "common/adt/span.hpp"
#include "common/assert.hpp"
#include "common/bitops.hpp"
#include "common/defs.hpp"
#include "common/math.hpp"

#include <algorithm>

namespace tiro {

/// A view that transforms a preallocated span of blocks into a bitset.
/// Contains search functions that are implemented in terms of compiler intrinsics that usually map
/// to hardware instructions.
template<typename BlockType>
class BitsetView final {
public:
    static_assert(std::is_integral_v<BlockType>, "block must be an integer");

    using index_type = size_t;
    using block_type = BlockType;

    static constexpr index_type npos = index_type(-1);

private:
    static constexpr size_t bits_per_block = type_bits<BlockType>();
    static_assert(is_pow2(bits_per_block));

    static constexpr size_t bits_per_block_log = log2(bits_per_block);

public:
    /// Constructs a new bitset view over the given amount of bits in `blocks`.
    /// `bits` must not exceed the actual size of `blocks` (in bits).
    explicit BitsetView(Span<BlockType> blocks, size_t bits)
        : blocks_(blocks)
        , bits_(bits) {
        TIRO_DEBUG_ASSERT(bits <= bits_per_block * blocks.size(), "invalid number of bits");
        TIRO_DEBUG_ASSERT(bits < npos, "number of bits is too large");
    }

    /// Returns the number of bits in the set.
    size_t size() const { return bits_; }

    /// Returns the number of *set* bits in the set.
    size_t count() const { return count(0, bits_); }

    /// Returns the number of *set* bits, starting with `begin`.
    /// \pre `begin <= size()`.
    size_t count(size_t begin) const {
        TIRO_DEBUG_ASSERT(begin <= size(), "begin value out of bounds");
        return count(begin, size() - begin);
    }

    /// Returns the number of *set* bits in the range [begin, begin + n).
    /// \pre `[begin, begin + n)` must be a valid sub range.
    size_t count(size_t begin, size_t n) const {
        TIRO_DEBUG_ASSERT(begin <= size(), "begin value out of bounds");
        TIRO_DEBUG_ASSERT(n <= size() - begin, "range size out of bounds");
        if (begin >= bits_ || n == 0)
            return 0;

        // Sets all bits before `index` to zero.
        auto mask_front = [](block_type block, u32 index) { return (block >> index) << index; };

        // Sets all bits at `index` and after it to zero.
        auto mask_back = [](block_type block, u32 index) {
            u32 offset = bits_per_block - index;
            return (block << offset) >> offset;
        };

        size_t end = begin + n;
        size_t current_block = block_index(begin);
        size_t last_block = block_index(end);
        size_t result = 0;

        // Handle the first block.
        if (u32 i = block_offset(begin); i != 0) {
            block_type blk = blocks_[current_block];
            blk = mask_front(blk, i);
            if (last_block == current_block) {
                u32 j = block_offset(end);
                blk = mask_back(blk, j);
                return popcount(blk);
            }

            result += popcount(blk);
            ++current_block;
        }

        // Blockwise popcount for all blocks until the last one is reached.
        for (; current_block < last_block; ++current_block) {
            result += popcount(blocks_[current_block]);
        }

        // Handle remainder in the last block.
        if (u32 i = block_offset(end); i != 0) {
            result += popcount(mask_back(blocks_[current_block], i));
        }

        return result;
    }

    /// Finds the position of the first set bit, starting from `begin` (inclusive).
    /// Returns the index of that bit or `npos` if none was found.
    /// \pre `begin <= size()`.
    ///
    /// TODO: Overload that supports a limit parameter `n`?
    size_t find_set(size_t begin = 0) const {
        TIRO_DEBUG_ASSERT(begin <= size(), "begin value out of bounds");
        if (begin >= bits_)
            return npos;

        size_t current_block = block_index(begin);
        size_t total_blocks = block_count();

        // Check current block if not on block boundary.
        if (u32 i = block_offset(begin); i != 0) {
            u32 s = find_first_set(blocks_[current_block] >> i);
            if (s != 0)
                return begin + s - 1;
            ++current_block;
        }

        while (current_block < total_blocks) {
            u32 s = find_first_set(blocks_[current_block]);
            if (s != 0)
                return (current_block << bits_per_block_log) + s - 1;
            ++current_block;
        }
        return npos;
    }

    /// Finds the position of the fist unset bit, starting from `begin` (inclusive).
    /// Returns the index of that bit or `npos` if none was found.
    /// \pre `begin <= size()`.
    ///
    /// TODO: Overload that supports a limit parameter `n`?
    size_t find_unset(size_t begin = 0) const {
        if (begin >= bits_)
            return npos;

        size_t result = [&] {
            size_t current_block = block_index(begin);
            size_t total_blocks = block_count();

            // Check current block if not on block boundary.
            if (u32 i = block_offset(begin); i != 0) {
                u32 s = find_first_set((~blocks_[current_block]) >> i);
                if (s != 0)
                    return begin + s - 1;
                ++current_block;
            }

            while (current_block < total_blocks) {
                u32 s = find_first_set(~blocks_[current_block]);
                if (s != 0) {
                    return current_block * bits_per_block + s - 1;
                }

                ++current_block;
            }
            return npos;
        }();

        // The last few bits beyond the size are always unset.
        return result < bits_ ? result : npos;
    }

    /// Resets all bits to 0.
    void clear() const { std::fill_n(blocks_.begin(), block_count(), 0); }

    /// Inverts all bits.
    void flip() const {
        for (auto& block : blocks_.first(block_count()))
            block = ~block;
    }

    /// Returns true if the bit at `index` is set, false otherwise.
    /// \pre `index < size()`.
    bool test(size_t index) const {
        TIRO_DEBUG_ASSERT(index < size(), "index out of bounds");
        return blocks_[block_index(index)] & (block_type(1) << block_offset(index));
    }

    /// Sets the bit at `index` to 1.
    /// \pre `index < size()`.
    void set(size_t index) const {
        TIRO_DEBUG_ASSERT(index < size(), "index out of bounds");
        blocks_[block_index(index)] |= (block_type(1) << block_offset(index));
    }

    /// Sets the bit at `index` to 0.
    /// \pre `index < size()`.
    void clear(size_t index) const {
        TIRO_DEBUG_ASSERT(index < size(), "index out of bounds");
        blocks_[block_index(index)] &= ~(block_type(1) << block_offset(index));
    }

    /// Sets or clears the bit at `index`, depending on `value`.
    /// \pre `index < size()`.
    void set(size_t index, bool value) const {
        if (value) {
            this->set(index);
        } else {
            this->clear(index);
        }
    }

    /// Inverts the value of the bit at `index`.
    /// \pre `index < size()`.
    void flip(size_t index) const { set(index, !test(index)); }

private:
    static size_t block_index(size_t bit_index) { return bit_index >> bits_per_block_log; }
    static size_t block_offset(size_t bit_index) { return bit_index & (bits_per_block - 1); }

    size_t block_count() const {
        size_t blocks = block_index(bits_);
        if (block_offset(bits_) != 0)
            ++blocks;
        return blocks;
    }

private:
    Span<BlockType> blocks_;
    size_t bits_;
};

/// A resizable bitset.
/// TODO: Small buffer impl (-> small vec?)
class DynamicBitset final {
public:
    using block_type = u64;

private:
    using view_type = BitsetView<block_type>;

public:
    using index_type = view_type::index_type;

    static constexpr index_type npos = view_type::npos;

public:
    explicit DynamicBitset(index_type size)
        : blocks_()
        , bits_(size) {
        resize(bits_);
    }

    void resize(index_type new_size) {
        TIRO_DEBUG_ASSERT(new_size < npos, "invalid number of bits");
        blocks_.resize(ceil_div(new_size, type_bits<block_type>()));
        bits_ = new_size;
    }

    /// Returns the number of bits in the set.
    size_t size() const { return bits_; }

    /// Returns the number of *set* bits in the set.
    size_t count() const { return view().count(); }

    /// Returns the number of *set* bits, starting with `begin`.
    /// \pre `begin <= size()`.
    size_t count(size_t begin) const { return view().count(begin); }

    /// Returns the number of *set* bits in the range [begin, begin + n).
    /// \pre `[begin, begin + n)` must be a valid sub range.
    size_t count(size_t begin, size_t n) const { return view().count(begin, n); }

    /// Finds the position of the first set bit, starting from `begin` (inclusive).
    /// Returns the index of that bit or `npos` if none was found.
    /// \pre `begin <= size()`.
    size_t find_set(size_t begin = 0) const { return view().find_set(begin); }

    /// Finds the position of the fist unset bit, starting from `begin` (inclusive).
    /// Returns the index of that bit or `npos` if none was found.
    /// \pre `begin <= size()`.
    ///
    /// TODO: Overload that supports a limit parameter `n`?
    size_t find_unset(size_t begin = 0) const { return view().find_unset(begin); }

    /// Resets all bits to 0.
    void clear() { return view().clear(); }

    /// Inverts all bits.
    void flip() { return view().flip(); }

    /// Returns true if the bit at `index` is set, false otherwise.
    /// \pre `index < size()`.
    bool test(size_t index) const { return view().test(index); }

    /// Sets the bit at `index` to 1.
    /// \pre `index < size()`.
    void set(size_t index) const { return view().set(index); }

    /// Sets the bit at `index` to 0.
    /// \pre `index < size()`.
    void clear(size_t index) const { return view().clear(index); }

    /// Sets or clears the bit at `index`, depending on `value`.
    /// \pre `index < size()`.
    void set(size_t index, bool value) const { return view().set(index, value); }

    /// Inverts the value of the bit at `index`.
    /// \pre `index < size()`.
    void flip(size_t index) const { view().flip(index); }

    /// Returns a span over the raw blocks of this bitset.
    Span<const block_type> raw_blocks() const { return Span(blocks_); }

private:
    view_type view() const { return view_type(Span(blocks_), bits_); }

private:
    // Mutable for easier integration with BitsetView, which requires a writable span.
    mutable std::vector<block_type> blocks_;
    size_t bits_;
};

} // namespace tiro

#endif // TIRO_COMMON_ADT_BITSET_HPP
