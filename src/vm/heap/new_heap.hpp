#ifndef TIRO_VM_HEAP_NEW_HEAP_HPP
#define TIRO_VM_HEAP_NEW_HEAP_HPP

#include "common/adt/not_null.hpp"
#include "common/adt/span.hpp"
#include "common/defs.hpp"
#include "common/math.hpp"
#include "vm/heap/fwd.hpp"
#include "vm/heap/memory.hpp"

#include "absl/container/flat_hash_set.h"

namespace tiro::vm::new_heap {

/// The size of a cell, in bytes. Cells are the smallest unit of allocation
/// in the vm's managed heap.
inline constexpr size_t cell_size = 2 * sizeof(void*);

/// Guaranteed alignment of objects, in bytes.
/// Note: object with higher alignment requirements cannot be allocated at the moment.
inline constexpr size_t cell_align = cell_size;

/// The number of available tag bits in any pointer allocated from the heap.
inline constexpr size_t cell_align_bits = log2(cell_size);

/// Represents a cell in a page.
struct alignas(cell_size) Cell final {
    //  Cell is never instantiated. It is only used for pointer arithmetic.
    Cell() = delete;

    byte data[cell_size];
};

/// Node in the free list.
/// Unused cells in a page may be used for these entries.
struct FreeListEntry {
    /// Points to the next free list entry.
    /// Nullptr at the end of the list.
    FreeListEntry* next = nullptr;

    /// Size of the current block, in cells.
    /// Includes the entry itself.
    size_t cells;

    explicit FreeListEntry(size_t cells_)
        : cells(cells_) {}

    explicit FreeListEntry(FreeListEntry* next_, size_t cells_)
        : next(next_)
        , cells(cells_) {}
};

static_assert(alignof(FreeListEntry) <= alignof(Cell));

/// Allocator interface used to allocate aligned pages and large object chunks.
/// TODO: Should probably move to a more central location?
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

/// Represents the type of a heap allocated chunk.
enum class ChunkType {
    /// Pages are large, size aligned chunks used for most object allocations.
    Page,

    /// Large object chunks contain a single large object that does not fit well into a page.
    /// They do not have a specific alignment.
    LargeObject,
};

/// Common base class of page and large object chunk.
class alignas(cell_size) Chunk {
public:
    explicit Chunk(ChunkType type, Heap& heap)
        : type_(type)
        , heap_(heap) {}

    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    /// Returns the type of this chunk.
    ChunkType type() const { return type_; }

    /// Returns the heap that this chunk belongs to.
    Heap& heap() const { return heap_; }

private:
    ChunkType type_;
    Heap& heap_;
};

/// Runtime values that determine the page layout.
/// Computed once, then cached.
///
/// Note: some of these values may be very fast to compute on the fly,
/// saving some space (-> cache locality) in this hot datastructure.
struct PageLayout {
    /// The size of all pages in the heap, in bytes.
    /// Always a power of two.
    u32 page_size;

    /// log2(page_size).
    u32 page_size_log;

    /// This mask can be applied (via bitwise AND) to pointers within a page to
    /// round down to the start of a page.
    constexpr uintptr_t page_mask() const { return aligned_container_mask(page_size); }

    /// The start of the block bitmap in a page (in bytes).
    u32 block_bitmap_offset;

    /// The start of the mark bitmap in a page (in bytes).
    u32 mark_bitmap_offset;

    /// The number of bitset items in a page.
    /// Note that bitset items are chunks of bits (e.g. u32).
    /// The actual number of readable bits is the same as `cells_size`.
    u32 bitmap_items;

    /// The start of the cells array in a page (in bytes).
    u32 cells_offset;

    /// The number of cells in a page.
    u32 cells_size;
};

/// Page are used to allocate most objects.
///
/// Internal page layout:
/// - Header (the Page class itself), aligned to CellSize
/// - Block bitmap (integer array), aligned to CellSize
/// - Mark bitmap, same size (integer array), aligned to CellSize
/// - Array of cells, aligned to CellSize
class alignas(cell_size) Page final : public Chunk {
public:
    // TODO: bitset algorithms and compiler intrinsics
    using BitsetItem = u32;

    static constexpr size_t min_size = 1 << 16;
    static constexpr size_t max_size = 1 << 24;
    static constexpr size_t default_size = 1 << 20;

    /// Calculates page layout depending on the user chosen parameters.
    /// Throws if `page_size` is not a power of two.
    static PageLayout compute_layout(size_t page_size);

    /// Returns a pointer to the page that contains this address.
    /// \pre the object referenced by `address` MUST be allocated from a page.
    static NotNull<Page*> from_address(const void* address, Heap& heap);
    static NotNull<Page*> from_address(const void* address, const PageLayout& layout);

    explicit Page(Heap& heap)
        : Chunk(ChunkType::Page, heap) {}

    /// Returns a span over this page's block bitmap.
    Span<BitsetItem> block_bitmap();

    /// Returns a span over this page's mark bitmap.
    Span<BitsetItem> mark_bitmap();

    /// Returns a span over this page's cell array.
    Span<Cell> cells();

    /// Returns the number of available cells in this page.
    size_t cells_count() { return layout().cells_size; }

    /// Returns the cell index of the first cell that belongs to this object.
    /// \pre the object referenced by `address` MUST be allocated from _this_ page.
    u32 cell_index(const void* address);

    /// Returns the page's layout descriptor.
    const PageLayout& layout() const;
};

/// The heap manages all memory dynamically allocated by the vm.
class Heap final {
public:
    explicit Heap(size_t page_size, HeapAllocator& alloc);
    ~Heap();

    Heap(const Heap&) = delete;
    Heap& operator=(const Heap&) = delete;

    HeapAllocator& alloc() const { return alloc_; }
    const PageLayout& layout() const { return layout_; }

    /// Attempts to allocate the given amount of bytes.
    /// Returns a pointer on success.
    /// Throws `std::bad_alloc` when no storage is available.
    /// TODO:
    /// - Garbage collection
    /// - Max size?
    void* allocate(size_t bytes);

private:
    NotNull<Page*> create_page();
    void destroy_page(NotNull<Page*> page);

private:
    HeapAllocator& alloc_;
    const PageLayout layout_;
    absl::flat_hash_set<NotNull<Page*>> pages_;
};

} // namespace tiro::vm::new_heap

#endif // TIRO_VM_HEAP_NEW_HEAP_HPP
