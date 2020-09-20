#include "vm/heap/memory.hpp"

#include <cstdlib>

namespace tiro::vm {

#if defined(__APPLE__)

// Posix based implementation.

static void* allocate_aligned_impl(size_t size, size_t alignment) {
    void* block;
    int result = posix_memalign(&block, alignment, size);
    if (result != 0)
        throw std::bad_alloc();
    TIRO_DEBUG_ASSERT(block, "Must return a valid pointer if result was 0.");
    return block;
}

static void deallocate_aligned_impl(void* block, size_t size, size_t alignment) {
    (void) size;
    (void) alignment;
    ::free(block);
}

#elif defined(_MSC_VER)

// Implementation for windows

static void* allocate_aligned_impl(size_t size, size_t alignment) {
    void* block = ::_aligned_malloc(size, alignment);
    if (!block)
        throw std::bad_alloc();
    return block;
}

static void deallocate_aligned_impl(void* block, size_t size, size_t alignment) {
    (void) size;
    (void) alignment;
    ::_aligned_free(block);
}

#else

// C standard based implementation.

static void* allocate_aligned_impl(size_t size, size_t alignment) {
    void* block = ::aligned_alloc(alignment, size);
    if (!block)
        throw std::bad_alloc();
    return block;
}

static void deallocate_aligned_impl(void* block, size_t size, size_t alignment) {
    (void) size;
    (void) alignment;
    ::free(block);
}

#endif

// TODO: Allocator support, this just uses the stdlibs heap.
void* allocate_aligned(size_t size, size_t alignment) {
    TIRO_DEBUG_ASSERT(is_pow2(alignment), "The alignment must be a power of two.");
    TIRO_DEBUG_ASSERT(size >= alignment, "The size must be >= the alignment.");
    return allocate_aligned_impl(size, alignment);
}

void deallocate_aligned(void* block, size_t size, size_t alignment) {
    deallocate_aligned_impl(block, size, alignment);
}

} // namespace tiro::vm
