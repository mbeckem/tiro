#ifndef TIRO_VM_HEAP_NEW_HEAP_HPP
#define TIRO_VM_HEAP_NEW_HEAP_HPP

#include "common/adt/not_null.hpp"
#include "common/adt/span.hpp"
#include "common/defs.hpp"
#include "common/math.hpp"
#include "vm/heap/chunks.hpp"
#include "vm/heap/collector.hpp"
#include "vm/heap/common.hpp"
#include "vm/heap/fwd.hpp"
#include "vm/heap/header.hpp"
#include "vm/heap/memory.hpp"
#include "vm/object_support/fwd.hpp"

#include "absl/container/flat_hash_set.h"

namespace tiro::vm {

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
