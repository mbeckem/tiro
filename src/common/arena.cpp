#include "common/arena.hpp"

#include <new>
#include <utility>

namespace tiro {

Arena::Arena(Arena&& other) noexcept
    : min_block_size_(other.min_block_size_)
    , head_(std::exchange(other.head_, nullptr))
    , memory_used_(std::exchange(other.memory_used_, 0))
    , memory_total_(std::exchange(other.memory_total_, 0))
    , current_ptr_(std::exchange(other.current_ptr_, nullptr))
    , current_remaining_(std::exchange(other.current_remaining_, 0)) {}

void Arena::deallocate() noexcept {
    while (head_) {
        auto block = head_;
        head_ = block->next_;
        std::free(block);
    }

    memory_used_ = 0;
    memory_total_ = 0;
    current_ptr_ = nullptr;
    current_remaining_ = 0;
}

void* Arena::allocate_slow_path(size_t size, [[maybe_unused]] size_t align) {
    Block* block = allocate_block(size);
    TIRO_DEBUG_ASSERT(block->data_size() >= size, "Arena: Allocated block is too small.");
    block->next_ = head_;
    head_ = block;
    current_ptr_ = block->data();
    current_remaining_ = block->data_size();

    if (!checked_add(memory_used_, size))
        throw std::bad_alloc();

    void* result = current_ptr_;
    TIRO_DEBUG_ASSERT(is_aligned(result, align), "Arena: Pointer is not aligned.");

    current_ptr_ = reinterpret_cast<byte*>(current_ptr_) + size;
    current_remaining_ -= size;
    return result;
}

Arena::Block* Arena::allocate_block(size_t min_data_size) {
    if (!checked_add(min_data_size, sizeof(Block)))
        throw std::bad_alloc();

    size_t alloc_size = round_block_size(min_data_size);
    void* block_storage = std::malloc(alloc_size);
    if (!block_storage)
        throw std::bad_alloc();

    memory_total_ += alloc_size;

    Block* block = new (block_storage) Block(alloc_size);
    return block;
}

size_t Arena::round_block_size(size_t size) {
    size_t div = 1 + (size - 1) / min_block_size_;
    size_t result;
    if (!checked_mul(div, min_block_size_, result))
        throw std::bad_alloc();
    return result;
}

} // namespace tiro
