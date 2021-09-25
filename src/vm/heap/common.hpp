#ifndef TIRO_VM_HEAP_COMMON_HPP
#define TIRO_VM_HEAP_COMMON_HPP

#include "common/defs.hpp"
#include "common/math.hpp"

namespace tiro::vm {

/// The size of a cell, in bytes. Cells are the smallest unit of allocation
/// in the vm's managed heap.
inline constexpr size_t cell_size = 2 * sizeof(void*);

/// Guaranteed alignment of objects, in bytes.
/// Note: objects with higher alignment requirements cannot be allocated at this time.
inline constexpr size_t cell_align = cell_size;

/// The number of available (least significant) tag bits in any pointer allocated from the heap.
inline constexpr size_t cell_align_bits = log2(cell_size);

/// Represents a cell in a page.
/// The cell type is never instantiated. It is only used for pointer arithmetic.
struct alignas(cell_size) Cell final {
    Cell() = delete;
    Cell(const Cell&) = delete;
    Cell& operator=(const Cell&) = delete;

private:
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunused-private-field"
#endif
    byte data[cell_size];
#if defined(__clang__)
#    pragma clang diagnostic pop
#endif
};

} // namespace tiro::vm

#endif // TIRO_VM_HEAP_COMMON_HPP
