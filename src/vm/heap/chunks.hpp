#ifndef TIRO_VM_HEAP_CHUNKS_HPP
#define TIRO_VM_HEAP_CHUNKS_HPP

#include "common/adt/bitset.hpp"
#include "common/adt/not_null.hpp"
#include "common/adt/span.hpp"
#include "common/defs.hpp"
#include "vm/heap/common.hpp"
#include "vm/heap/fwd.hpp"
#include "vm/heap/memory.hpp"

#include "absl/container/flat_hash_set.h"

namespace tiro::vm {

/// Represents the type of a heap allocated chunk.
enum class ChunkType : u8 {
    /// Pages are large, size aligned chunks used for most object allocations.
    Page,

    /// Large object chunks contain a single large object that does not fit well into a page.
    /// They do not have a specific alignment.
    LargeObject,
};

/// Common base class of page and large object chunk.
class alignas(cell_size) Chunk {
public:
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    /// Returns the type of this chunk.
    ChunkType type() const { return type_; }

    /// Returns the heap that this chunk belongs to.
    Heap& heap() const { return heap_; }

protected:
    explicit Chunk(ChunkType type, Heap& heap)
        : type_(type)
        , heap_(heap) {}

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

    /// Minimum number of cells for large objects.
    /// Objects smaller than this are allocated from normal pages.
    u32 large_object_cells;
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
    using BitsetItem = u32;

    static constexpr size_t min_size_bytes = 1 << 16;
    static constexpr size_t max_size_bytes = 1 << 24;
    static constexpr size_t default_size_bytes = 1 << 20;

    /// Calculates page layout depending on the user chosen parameters.
    /// Throws if `page_size` is not a power of two.
    static PageLayout compute_layout(size_t page_size);

    /// Returns a pointer to the page that contains this address.
    /// \pre the object referenced by `address` MUST be allocated from a page.
    static NotNull<Page*> from_address(const void* address, Heap& heap);
    static NotNull<Page*> from_address(const void* address, const PageLayout& layout);

    /// Allocates a page for the provided heap, using the heap's allocator and page layout.
    static NotNull<Page*> allocate(Heap& heap);

    /// Destroys a page.
    static void destroy(NotNull<Page*> page);

    /// Returns a view over this page's block bitmap.
    BitsetView<BitsetItem> block_bitmap();

    /// Returns a view over this page's mark bitmap.
    BitsetView<BitsetItem> mark_bitmap();

    struct SweepStats {
        u32 allocated_cells = 0;
        u32 free_cells = 0;
    };

    /// Sweeps this page after the heap was traced. Invoked by the garbage collector.
    ///
    /// Visits all unmarked (dead) blocks in this page, coalesces neighboring free blocks,
    /// and then registers them with the free space.
    /// Marked (live) blocks are not touched.
    /// As a side effect, all blocks within this page are reset to `unmarked`.
    void sweep(SweepStats& stats, FreeSpace& free_space);

    /// Invoke the finalizers of unmarked objects.
    /// This is usually called automatically from sweep(), but it is also called directly
    /// when the heap is shutting down.
    void invoke_finalizers();

    /// Returns a span over this page's cell array.
    Span<Cell> cells();

    /// Returns the cell with the given index.
    /// \pre `index < cells_count()`.
    Cell* cell(u32 index);

    /// Returns the number of available cells in this page.
    u32 cells_count() { return layout().cells_size; }

    /// Returns the cell index of the first cell that belongs to this object.
    /// \pre the object referenced by `address` MUST be allocated from _this_ page.
    u32 cell_index(const void* address);

    /// Returns true if the cell has been marked already, false otherwise.
    ///
    /// \pre
    ///     This operation may only be used if the cell belongs to an allocated object
    ///     and if the cell is the *first* cell in that object.
    ///     Consult the bitmap table in `design/heap.md` for more information.
    bool is_cell_marked(u32 index);

    /// Sets this cell to marked. This method is used by the garbage collector.
    /// \pre
    ///     This operation may only be used if the cell belongs to an allocated object
    ///     and if the cell is the *first* cell in that object.
    ///     Consult the bitmap table in `design/heap.md` for more information.
    void set_cell_marked(u32 index, bool marked);

    /// Returns true if the given cell is the start of an allocated block, i.e. not on the free list
    /// and not a block extent.
    /// NOTE: Consult the bitmap table in `design/heap.md` for more information.
    bool is_allocated_block_start(u32 index);

    /// Returns true if the given cell is the start of a free block, in which case it should
    /// also be on the free list.
    /// NOTE: Consult the bitmap table in `design/heap.md` for more information.
    bool is_free_block_start(u32 index);

    /// Returns true if the given cell is a continuation block.
    /// NOTE: Consult the bitmap table in `design/heap.md` for more information.
    bool is_cell_block_extent(u32 index);

    /// Marks the cell range [index, index + size) as allocated in the block & mark bitmaps.
    /// Any previous bitmap state of the cell range is discarded; no invariants are checked.
    void set_allocated(u32 index, u32 size);

    /// Marks the cell range [index, index + size) as free in the block & mark bitmaps.
    /// Any previous bitmap state of the cell range is discarded; no invariants are checked.
    void set_free(u32 index, u32 size);

    /// Marks the cell as containing an object with a finalizer.
    void mark_finalizer(u32 index);

    /// Computes the size of the block that starts with the cell at `index`, by counting
    /// the number of block extent cells after the given cell.
    /// Mainly used for debugging (not optimized).
    ///
    /// \pre Must be the start index of a block (this is not checked, not even via an assertion).
    u32 get_block_extent(u32 index);

    /// Returns the page's layout descriptor.
    const PageLayout& layout() const;

private:
    explicit Page(Heap& heap);
    ~Page();

    Span<BitsetItem> block_bitmap_storage();
    Span<BitsetItem> mark_bitmap_storage();

private:
    // Set of cell indices that contain objects that must be finalized.
    absl::flat_hash_set<u32> finalizers_;
};
static_assert(alignof(Page) == cell_align);

/// Provides storage for a single large object that does not fit into a page.
class alignas(cell_size) LargeObject final : public Chunk {
public:
    /// Returns a pointer to the large object chunk that contains this address.
    /// \pre the object referenced by `address` MUST be allocated as a large object.
    static NotNull<LargeObject*> from_address(const void* address);

    /// Allocates a new large object chunk for the given heap.
    /// The chunk will have exactly `cells_count` cells available.
    static NotNull<LargeObject*> allocate(Heap& heap, u32 cells_count);

    /// Returns the number of bytes that must be allocated to accommodate the given amount of cells.
    static size_t dynamic_size(u32 cells);

    /// Destroys a large object chunk.
    static void destroy(NotNull<LargeObject*> lob);

    /// Returns a span over the object stored in this chunk.
    Span<Cell> cells();

    /// Returns the address of the object's first cell.
    Cell* cell();

    /// Returns the number of cells allocated directly after this chunk header.
    size_t cells_count() const { return cells_count_; }

    /// Returns true if this object has been marked.
    bool is_marked();

    /// Sets the 'marked' value.
    void set_marked(bool value);

    /// Returns true if this object has a finalizer.
    bool has_finalizer() const;

    /// Sets the 'has_finalizer' flag.
    void set_finalizer(bool has_finalizer);

    /// Returns the dynamic size of this object.
    size_t dynamic_size() { return LargeObject::dynamic_size(cells_count_); }

    /// Runs the finalizer if necessary.
    /// Called during sweep or when the heap is destroyed.
    void invoke_finalizer();

private:
    explicit LargeObject(Heap& heap, u32 cells)
        : Chunk(ChunkType::LargeObject, heap)
        , cells_count_(cells) {}

private:
    bool marked_ = false;
    bool finalizer_ = false;
    u32 cells_count_;
};
static_assert(alignof(LargeObject) == cell_align);

} // namespace tiro::vm

#endif // TIRO_VM_HEAP_CHUNKS_HPP
