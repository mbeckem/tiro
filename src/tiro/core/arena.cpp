#include "tiro/core/arena.hpp"

#include <new>
#include <utility>

namespace tiro {

Arena::Arena(Arena&& other) noexcept
    : min_block_size_(other.min_block_size_)
    , blocks_(std::move(other.blocks_))
    , memory_used_(std::exchange(other.memory_used_, 0))
    , memory_total_(std::exchange(other.memory_total_, 0))
    , current_ptr_(std::exchange(other.current_ptr_, nullptr))
    , current_remaining_(std::exchange(other.current_remaining_, 0)) {
    other.blocks_.clear();
}

void Arena::deallocate() noexcept {
    blocks_.clear_and_dispose([](Block* blk) { std::free(blk); });

    memory_used_ = 0;
    memory_total_ = 0;
    current_ptr_ = nullptr;
    current_remaining_ = 0;
}

void* Arena::allocate_slow_path(size_t size, [[maybe_unused]] size_t align) {
    Block* blk = allocate_block(size);
    TIRO_DEBUG_ASSERT(
        blk->data_size() >= size, "Arena: Allocated block is too small.");
    blocks_.push_back(*blk);

    current_ptr_ = blk->data();
    current_remaining_ = blk->data_size();

    if (!checked_add(memory_used_, size))
        throw std::bad_alloc();

    void* result = current_ptr_;
    TIRO_DEBUG_ASSERT(
        is_aligned(result, align), "Arena: Pointer is not aligned.");

    current_ptr_ = reinterpret_cast<byte*>(current_ptr_) + size;
    current_remaining_ -= size;
    return result;
}

Arena::Block* Arena::allocate_block(size_t size) {
    if (!checked_add(size, sizeof(Block)))
        throw std::bad_alloc();
    size = round_block_size(size);

    void* block_storage = std::malloc(size);
    if (!block_storage)
        throw std::bad_alloc();

    memory_total_ += size;

    Block* blk = new (block_storage) Block(size);
    return blk;
}

size_t Arena::round_block_size(size_t size) {
    size_t div = 1 + (size - 1) / min_block_size_;
    size_t result;
    if (!checked_mul(div, min_block_size_, result))
        throw std::bad_alloc();
    return result;
}

} // namespace tiro
