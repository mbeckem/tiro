#ifndef TIRO_VM_HEAP_NEW_HEAP_HPP
#define TIRO_VM_HEAP_NEW_HEAP_HPP

#include "common/adt/not_null.hpp"
#include "common/defs.hpp"
#include "common/math.hpp"
#include "vm/heap/fwd.hpp"

#include "absl/container/flat_hash_set.h"

namespace tiro::vm::new_heap {

/// The size of a cell, in bytes. Cells are the smallest unit of allocation
/// in the vm's managed heap.
inline constexpr size_t cell_size = 2 * sizeof(void*);

/// Guaranteed alignment of objects, in bytes.
/// Note: object with higher alignment requirements cannot be allocated at the moment.
inline constexpr size_t object_align = cell_size;

/// The number of available tag bits in any pointer allocated from the heap.
inline constexpr size_t object_align_bits = log2(cell_size);

/// Runtime values that determine the heap layout.
/// Computed once, then cached.
struct HeapFacts {
    /// The size of all pages in the heap, in bytes.
    /// Always a power of two.
    size_t page_size;

    /// log2(page_size).
    size_t page_size_log;

    /// This mask can be applied (via bitwise AND) to pointers within a page to
    /// round down to the start of a page.
    uintptr_t page_mask;

    // TODO:
    // size_t cells_in_page
    // size_t cells_offset
    // size_t marking_bitset_offset

    /// Calculates heap facts depending on the user chosen parameters.
    /// Throws if `page_size` is not a power of two.
    explicit HeapFacts(size_t page_size_);
};

/// Allocator interface used to allocate aligned pages and large object chunks.
/// TODO: Should probably move to a more central location?
struct HeapAllocator {
    virtual ~HeapAllocator();

    /// Allocates a new block of the given size, with the specified alignment.
    /// Alignment is always a power of 2.
    /// Should return nullptr on allocation failure.
    virtual void* allocate_aligned(size_t size, size_t align) = 0;

    /// Frees a block of memory previously allocated via `allocate_aligned`.
    /// `size` and `align` are the exact arguments used when allocating the block.
    virtual void free_aligned(void* block, size_t size, size_t align) = 0;
};

/// The heap manages all memory dynamically allocated by the vm.
class Heap final {
public:
    explicit Heap(size_t page_size, HeapAllocator& alloc);
    ~Heap();

    Heap(const Heap&) = delete;
    Heap& operator=(const Heap&) = delete;

    HeapAllocator& alloc() const { return alloc_; }
    const HeapFacts& facts() const { return facts_; }

private:
    NotNull<Page*> create_page();
    void destroy_page(NotNull<Page*> page);

private:
    HeapAllocator& alloc_;
    const HeapFacts facts_;
    absl::flat_hash_set<NotNull<Page*>> pages_;
};

/// Common base class of page and large object chunk.
class alignas(cell_size) Chunk {
public:
    explicit Chunk(Heap& heap)
        : heap_(heap) {}

    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    Heap& heap() const { return heap_; }
    const HeapFacts& facts() const { return heap().facts(); }

private:
    Heap& heap_;
};

/// Page are used to allocate most objects.
class Page final : public Chunk {
public:
    /// Returns a pointer to the page that contains this address.
    /// \pre the object referenced by `address` MUST be allocated from a page.
    static NotNull<Page*> from_address(Heap& heap, const void* address);

    explicit Page(Heap& heap)
        : Chunk(heap) {}
};

} // namespace tiro::vm::new_heap

#endif // TIRO_VM_HEAP_NEW_HEAP_HPP
