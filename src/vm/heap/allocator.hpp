#ifndef TIRO_VM_HEAP_ALLOCATOR_HPP
#define TIRO_VM_HEAP_ALLOCATOR_HPP

#include "common/defs.hpp"

namespace tiro::vm {

/// Allocator interface used to allocate aligned pages and large object chunks.
class HeapAllocator {
public:
    virtual ~HeapAllocator();

    /// Allocates a new block of the given size, with the specified alignment.
    /// Alignment is always a power of 2.
    /// Should return nullptr on allocation failure.
    virtual void* allocate_aligned(size_t size, size_t align) = 0;

    /// Frees a block of memory previously allocated via `allocate_aligned`.
    /// `size` and `align` are the exact arguments used when allocating the block.
    virtual void free_aligned(void* block, size_t size, size_t align) = 0;
};

/// Default implementation of HeapAllocator that uses appropriate system
/// allocation functions for the current platform.
class DefaultHeapAllocator final : public HeapAllocator {
public:
    ~DefaultHeapAllocator();

    virtual void* allocate_aligned(size_t size, size_t align) override;
    virtual void free_aligned(void* block, size_t size, size_t align) override;
};

} // namespace tiro::vm

#endif // TIRO_VM_HEAP_ALLOCATOR_HPP
