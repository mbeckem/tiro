#ifndef TIRO_COMMON_ARENA_HPP
#define TIRO_COMMON_ARENA_HPP

#include "common/defs.hpp"
#include "common/iter_tools.hpp"
#include "common/math.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace tiro {

/// An arena allocates storage linearly from large chunks of memory.
/// Individual deallocation is not supported; storage must be deallocated all at once.
/// TODO: polymorphic_allocator support
class Arena final {
public:
    static constexpr size_t default_min_block_size = 4096;

public:
    /// Constructs a new arena. The min_block_size argument must be a power of 2.
    /// It should be larger than the largest "usual" allocation size made through the arena.
    explicit Arena(size_t min_block_size = default_min_block_size);

    ~Arena() { deallocate(); }

    Arena(Arena&& other) noexcept;

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    /// Allocates `size` bytes aligned to the given alignment. The alignment must
    /// be a power of 2 and must not be greater than alignof(std::max_align_t).
    ///
    /// 0 sized allocations are not supported.
    void* allocate(size_t size, size_t align = alignof(std::max_align_t));

    /// Deallocates all memory allocated by this arena.
    void deallocate() noexcept;

    /// Returns the number of used bytes (bytes requested by allocations).
    size_t used_bytes() const { return memory_used_; }

    /// Returns the total number of bytes allocated by this arena. This includes
    /// fragmentation between allocations that were neccessary because of alignment
    /// or because new blocks had to be allocated.
    size_t total_bytes() const { return memory_total_; }

    /// Returns the minimum block size used for block allocations.
    size_t min_block_size() const { return min_block_size_; }

private:
    /// Blocks hold the storage allocated by the arena. Blocks are linked together into a list.
    /// The data managed by a block starts immediately after the block instance.
    struct alignas(std::max_align_t) Block {
        Block* next_ = nullptr; // Intrusive list
        size_t size_;           // size in bytes, including sizeof(block)

        Block(size_t size)
            : size_(size) {}

        ~Block() = default;

        byte* data() { return reinterpret_cast<byte*>(this + 1); }
        size_t data_size() { return size_ - sizeof(Block); }
    };

private:
    void* allocate_slow_path(size_t size, size_t align);

    // Allocates a block with at least that many useable bytes for data.
    Block* allocate_block(size_t min_data_size);

    // Rounds data_size up to a multiple of the minimum block size.
    size_t round_block_size(size_t data_size);

    bool is_aligned(void* addr, size_t align) const noexcept {
        return tiro::is_aligned<std::uintptr_t>(reinterpret_cast<std::uintptr_t>(addr), align);
    }

private:
    // Allocate at least this much memory when we need new blocks.
    size_t min_block_size_;

    // Linked list of existing blocks.
    Block* head_ = nullptr;

    // Memory used for allocations.
    size_t memory_used_ = 0;

    // Total memory allocated (includes fragmentation).
    size_t memory_total_ = 0;

    // We're allocating at this position.
    void* current_ptr_ = nullptr;

    // Available bytes in this block, starting from current_ptr_.
    size_t current_remaining_ = 0;
};

inline Arena::Arena(size_t min_block_size)
    : min_block_size_(min_block_size) {
    TIRO_DEBUG_ASSERT(
        is_pow2(min_block_size), "Arena: The minimum block size must be a power of two.");
    TIRO_DEBUG_ASSERT(
        min_block_size_ >= sizeof(Block), "Arena: The minimum block size is too small.");
}

inline void* Arena::allocate(size_t size, size_t align) {
    TIRO_DEBUG_ASSERT(size > 0, "Arena: Zero sized allocation.");
    TIRO_DEBUG_ASSERT(is_pow2(align), "Arena: The alignment must be a power of 2.");
    TIRO_DEBUG_ASSERT(align <= alignof(std::max_align_t), "Arena: The alignment is too large.");

    // Fast path: allocate from the current block.
    if (void* result = std::align(align, size, current_ptr_, current_remaining_)) {
        if (!checked_add(memory_used_, size))
            throw std::bad_alloc();

        TIRO_DEBUG_ASSERT(is_aligned(result, align), "Arena: Pointer is not aligned.");
        current_ptr_ = reinterpret_cast<byte*>(current_ptr_) + size;
        current_remaining_ -= size;
        return result;
    }

    return allocate_slow_path(size, align);
}

} // namespace tiro

#endif // TIRO_COMMON_ARENA_HPP
