#include "vm/heap/allocator.hpp"

#include "vm/heap/memory.hpp"

namespace tiro::vm {

HeapAllocator::~HeapAllocator() {}

DefaultHeapAllocator::~DefaultHeapAllocator() {}

void* DefaultHeapAllocator::allocate_aligned(size_t size, size_t align) {
    return tiro::vm::allocate_aligned(size, align);
}

void DefaultHeapAllocator::free_aligned(void* block, size_t size, size_t align) {
    return tiro::vm::deallocate_aligned(block, size, align);
}

} // namespace tiro::vm
