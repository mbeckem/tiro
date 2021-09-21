#ifndef TIRO_VM_HEAP_MEMORY_HPP
#define TIRO_VM_HEAP_MEMORY_HPP

#include "common/defs.hpp"
#include "common/math.hpp"

namespace tiro::vm {

inline constexpr uintptr_t aligned_container_mask(size_t container_alignment) {
    TIRO_DEBUG_ASSERT(container_alignment > 0 && is_pow2(container_alignment),
        "Container alignment must be a power of two.");
    return ~(static_cast<uintptr_t>(container_alignment) - 1);
}

/// When used with aligned containers (structs with guaranteed alignment and size), this
/// function can be used to return a pointer to the outer container.
/// For example, when 4 KiB pages are used, one can find the start of the page from a given member
/// by rounding down to the next lower address divisible by 4 KiB.
///
/// The mask must be obtained by calling `aligned_container_mask` with the appropriate alignment.
inline void* aligned_container_from_member(void* member, uintptr_t container_mask) {
    // TODO: This is technically undefined behaviour, even though must platforms allow it.
    // Memory is not necessarily linear when reinterpreted as uintptr_t.
    uintptr_t raw_member = reinterpret_cast<uintptr_t>(member);
    return reinterpret_cast<void*>(raw_member & container_mask);
}

/// Allocates `size` bytes from the system heap.
/// The returned address will be aligned correctly w.r.t. `alignment`, which must be a power of two.
void* allocate_aligned(size_t size, size_t alignment);

/// Deallocates a block of memory previously allocated through `allocate_aligned()`.
/// Size and alignment must be the same as the arguments used during the initial allocation.
void deallocate_aligned(void* block, size_t size, size_t alignment);

} // namespace tiro::vm

#endif // TIRO_VM_HEAP_MEMORY_HPP
