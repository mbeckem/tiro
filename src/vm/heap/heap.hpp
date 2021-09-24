#ifndef TIRO_VM_HEAP_NEW_HEAP_HPP
#define TIRO_VM_HEAP_NEW_HEAP_HPP

#include "common/adt/bitset.hpp"
#include "common/adt/not_null.hpp"
#include "common/adt/span.hpp"
#include "common/defs.hpp"
#include "common/math.hpp"
#include "vm/heap/collector.hpp"
#include "vm/heap/fwd.hpp"
#include "vm/heap/header.hpp"
#include "vm/heap/memory.hpp"
#include "vm/object_support/fwd.hpp"

#include "absl/container/flat_hash_set.h"

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

/// Manages unallocated space on a series of free lists.
/// Memory registered with the free lists must not be used until it is removed again
/// because its storage will be used for linked lists.
///
/// Note that memory block sizes in this class are expressed in numbers of _cells_.
///
/// Size classes:
/// - size class `i` has the associated size `size[i]`.
/// - size class `i` contains all memory blocks with `block_size_in_cells >= size[i] && block_size_in_cells < size[i+1]`
/// - the last size class contains all larger memory blocks
///
/// NOTE: currently all pages share a global free space datastructure.
/// This reduces the per-page overhead but also makes handling individual pages impossible.
///
/// NOTE: all cells must come from a page with the expected layout.
///
/// TODO: Run tests on 32 bit?
class FreeSpace final {
public:
    /// NOTE: `layout` is captured by reference in this class.
    explicit FreeSpace(const PageLayout& layout);

    /// Allocates a span of exactly `count` cells.
    /// \pre `count > 0`
    /// \param count the required number of cells
    /// \returns a valid cell pointer or nullptr if the request cannot be fulfilled
    Cell* allocate_exact(u32 count);

    /// Attempts to allocate a chunk of at least `count` cells.
    /// May return significantly more cells to the caller.
    /// This function is suited to obtain large buffers for sequential (bump pointer) allocations.
    ///
    /// TODO: use this for bump pointer allocation
    /// TODO: must not be marked as allocated block when used for bump pointer allocation
    ///
    /// \pre `count > 0`.
    /// \param count the required number of cells
    /// \returns a valid span of at least `count` cells or an empty span if the allocation fails
    Span<Cell> allocate_chunk(u32 count);

    /// Inserts a block of free cells into the free space.
    /// This function is invoked by the garbage collector in order to (re-)register free blocks with the free space
    /// when sweeping the heap.
    ///
    /// \pre `cells.size() >= 1`.
    /// \pre cells must already be marked as a free block.
    void insert_free(Span<Cell> cells);

    /// Inserts a block of free cells into the free space.
    /// This function is like `insert_free`, but also marks the span of cells
    /// as a free block.
    ///
    /// \pre `cells.size() >= 1`.
    void insert_free_with_metadata(Span<Cell> cells);

    /// Drops all references to free blocks.
    void reset();

    /// Returns the size class index for the given allocation size in cells.
    /// Exposed for testing.
    /// \pre `alloc` must be >= 1.
    u32 class_index(u32 alloc) const;

    /// Returns the associated block size of the size class with the given index.
    /// Exposed for testing.
    /// \pre `index < class_count()`.
    u32 class_size(u32 index) const;

    /// Returns the total number of size classes.
    /// Exposed for testing.
    u32 class_count() const;

private:
    struct FreeList {
        FreeListEntry* head = nullptr;
    };

    static void push(FreeList& list, Span<Cell> cells);
    static Span<Cell> first_fit(FreeList& list, u32 count);
    static Span<Cell> pop(FreeList& list);

private:
    // Number of size classes with exact cell size: [1, 2, 3, ..., exact_size_classes)
    static constexpr u32 exact_size_classes = (256 / cell_size) - 1;

    // First power of two used for exponential size classes.
    static constexpr u32 first_exp_size_class = exact_size_classes + 1;
    static constexpr u32 first_exp_size_class_log = log2(first_exp_size_class);
    static_assert(is_pow2(first_exp_size_class));

    const PageLayout& layout_;

    // Number of size classes with exponential cell size:
    // - for odd indices, the size is a power of two
    // - for even indices, the size is 1.5 * the previous power of two
    u32 exp_size_classes_ = 0;

    // Free list headers (size is exact_size_classes_ + exp_size_classes.size()).
    std::vector<FreeList> lists_;
};

struct HeapStats {
    /// Total raw memory allocated by the heap, includes overhead for metadata.
    size_t total_bytes = 0;

    /// Memory handed out to the mutator for object storage.
    size_t allocated_bytes = 0;

    /// Total free memory (e.g. on free lists).
    size_t free_bytes = 0;

    /// Total number of allocated objects.
    /// Some of these objects may already be unreachable and will be removed
    /// from the count when the garbage collector runs again.
    size_t allocated_objects = 0;
};

/// The heap manages all memory dynamically allocated by the vm.
class Heap final {
public:
    static constexpr size_t max_allocation_size = 16 * (1 << 20);

    explicit Heap(size_t page_size, HeapAllocator& alloc);
    ~Heap();

    Heap(const Heap&) = delete;
    Heap& operator=(const Heap&) = delete;

    /// Returns the page layout for this heap.
    /// The layout is computed at heap construction time and shared by all pages of that heap.
    const PageLayout& layout() const { return layout_; }

    /// Returns heap usage statistics.
    const HeapStats& stats() const { return stats_; }

    Collector& collector() { return collector_; }

    /// Creates a new object using the given object layout descriptor.
    /// Returns either a valid pointer to a constructed object or throws an exception.
    ///
    /// May trigger a garbage collection cycle if deemed necessary.
    ///
    /// \pre `bytes > 0`
    template<typename Layout, typename... Args>
    inline Layout* create(size_t bytes, Args&&... args) {
        using Traits = LayoutTraits<Layout>;
        static_assert(std::is_base_of_v<Header, Layout>);

        TIRO_DEBUG_ASSERT(bytes >= sizeof(Layout),
            "allocation size is too small for instances of the given type");

        auto [storage, chunk_type] = allocate(bytes);

        Layout* result = new (storage) Layout(std::forward<Args>(args)...);
        if (chunk_type == ChunkType::LargeObject)
            static_cast<Header*>(result)->large_object(true);
        if constexpr (Traits::has_finalizer)
            mark_finalizer(chunk_type, result);

        TIRO_DEBUG_ASSERT((void*) result == (void*) static_cast<Header*>(result),
            "invalid location of header in struct");
        return result;
    }

    /// The maximum heap size. Defaults to 'unconstrained' (max size_t).
    size_t max_size() const { return max_size_; }

    /// Set the maximum heap size.
    void max_size(size_t max_size) { max_size_ = max_size; }

private:
    friend Collector;

    /// Called by the collector with the number of traced (live) objects.
    void update_allocated_objects(size_t count) { stats_.allocated_objects = count; }

    /// Called by the collector after tracing the heap.
    void sweep();

private:
    /// Attempts to allocate the given amount of bytes.
    /// May trigger garbage collection when necessary.
    ///
    /// The caller is responsible to create a valid object in the provided storage immediately.
    ///
    /// Returns a pointer on success.
    /// Throws a `tiro::Error` with code `TIRO_ERROR_ALLOC` on error.
    ///
    /// \pre `bytes > 0`
    std::tuple<void*, ChunkType> allocate(size_t bytes);

    /// Marks an object has having a finalizer.
    /// Must be called after allocate (when the object has been constructed) or not at all.
    /// \pre `address` points to a valid object.
    void mark_finalizer(ChunkType chunk, void* address);

    // Allocates and registers.
    NotNull<Page*> add_page();
    NotNull<LargeObject*> add_lob(u32 cells);

    // Must already be unregistered.
    void destroy_lob(NotNull<LargeObject*> lob);

private:
    friend LargeObject;
    friend Page;

    void* allocate_raw(size_t size, size_t align);
    void free_raw(void* block, size_t size, size_t align);

private:
    HeapAllocator& alloc_;
    const PageLayout layout_;
    Collector collector_;
    FreeSpace free_;
    absl::flat_hash_set<NotNull<Page*>> pages_;
    absl::flat_hash_set<NotNull<LargeObject*>> lobs_;
    HeapStats stats_;
    size_t max_size_ = size_t(-1);
};

} // namespace tiro::vm

#endif // TIRO_VM_HEAP_NEW_HEAP_HPP
