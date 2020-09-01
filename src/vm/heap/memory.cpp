#include "vm/heap/memory.hpp"

#include <cstdlib>

namespace tiro::vm {

// TODO: Allocator support, this just uses the stdlibs heap.
void* allocate_aligned(size_t size, size_t alignment) {
    TIRO_DEBUG_ASSERT(is_pow2(alignment), "The alignment must be a power of two.");
    TIRO_DEBUG_ASSERT(size >= alignment, "The size must be >= the alignment.");
    void* block = ::aligned_alloc(alignment, size);
    if (!block)
        throw std::bad_alloc();
    return block;
}

void deallocate_aligned(
    void* block, [[maybe_unused]] size_t size, [[maybe_unused]] size_t alignment) {
    ::free(block);
}

} // namespace tiro::vm
