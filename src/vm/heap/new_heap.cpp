#include "vm/heap/new_heap.hpp"

#include <algorithm>
#include <new>

#include "fmt/format.h"

#include "common/scope_guards.hpp"
#include "vm/heap/memory.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm::new_heap {

#if 0
#    define TIRO_TRACE_FREE_SPACE(...) fmt::print("free space: " __VA_ARGS__);
#else
#    define TIRO_TRACE_FREE_SPACE(...)
#endif

HeapAllocator::~HeapAllocator() {}

DefaultHeapAllocator::~DefaultHeapAllocator() {}

void* DefaultHeapAllocator::allocate_aligned(size_t size, size_t align) {
    return tiro::vm::allocate_aligned(size, align);
}

void DefaultHeapAllocator::free_aligned(void* block, size_t size, size_t align) {
    return tiro::vm::deallocate_aligned(block, size, align);
}

NotNull<Page*> Page::from_address(const void* address, Heap& heap) {
    return from_address(address, heap.layout());
}

NotNull<Page*> Page::from_address(const void* address, const PageLayout& layout) {
    TIRO_DEBUG_ASSERT(address, "invalid address");
    return TIRO_NN(reinterpret_cast<Page*>(
        aligned_container_from_member(const_cast<void*>(address), layout.page_mask())));
}

PageLayout Page::compute_layout(size_t page_size) {
    if (page_size < Page::min_size_bytes || page_size > Page::max_size_bytes)
        TIRO_ERROR("page size must be in the range [{}, {}]: {}", Page::min_size_bytes,
            Page::max_size_bytes, page_size);

    if (!is_pow2(page_size))
        TIRO_ERROR("page size must be a power of two: {}", page_size);

    PageLayout layout;
    layout.page_size = page_size;
    layout.page_size_log = log2_fast(page_size);

    const size_t P = page_size;
    const size_t H = sizeof(Page);
    const size_t C = cell_size;

    // Original equation, where N is the number of cells:
    //
    //      H  +  2 * [(N + 8*C - 1) / (8*C)] * C  +  N*C  <=  P
    //
    // The number of bits in the bitset is rounded up to a multiple of C for simplicity.
    // We use multiplies of C for both bitsets, wasting a bit of space; also for simplicity.
    const size_t N = (4 * (P - H - 2 * C) + 1) / (1 + 4 * C);

    // The bitset's size is a multiple of the cell size, so we place multiple
    // items at once.
    static_assert(alignof(Page) == cell_size);
    static_assert(is_aligned(cell_size, alignof(BitsetItem)),
        "BitSetItem alignment must fit into object alignment");
    const size_t B = ((N + 8 * C - 1) / (8 * C)) * C;
    TIRO_DEBUG_ASSERT(B % C == 0, "bitset size must be a multiple of the cell size");
    TIRO_DEBUG_ASSERT(
        B % sizeof(BitsetItem) == 0, "bitset size must be a multiple of the item size");
    TIRO_DEBUG_ASSERT(B * 8 >= C, "bitset must have enough bits for all cells");

    layout.block_bitmap_offset = sizeof(Page);
    layout.mark_bitmap_offset = layout.block_bitmap_offset + B;
    layout.bitmap_items = B / sizeof(BitsetItem);
    layout.cells_offset = layout.mark_bitmap_offset + B;
    layout.cells_size = N;
    layout.large_object_cells = N / 4;
    return layout;
}

NotNull<Page*> Page::allocate(Heap& heap) {
    auto& layout = heap.layout();
    void* block = heap.allocate_raw(layout.page_size, layout.page_size);
    return TIRO_NN(new (block) Page(heap));
}

void Page::destroy(NotNull<Page*> page) {
    auto& heap = page->heap();
    auto& layout = heap.layout();
    page->~Page();
    heap.free_raw(static_cast<void*>(page.get()), layout.page_size, layout.page_size);
}

BitsetView<Page::BitsetItem> Page::block_bitmap() {
    auto storage = block_bitmap_storage();
    return BitsetView(storage, cells_count());
}

BitsetView<Page::BitsetItem> Page::mark_bitmap() {
    auto storage = mark_bitmap_storage();
    return BitsetView(storage, cells_count());
}

void Page::sweep(SweepStats& stats, FreeSpace& free_space) {
    // Invoke all finalizers for objects that have not been marked.
    // This is not very efficient (improvement: separate pages for objects with finalizers?)
    // but it will do for now.
    for (auto it = finalizers_.begin(), end = finalizers_.end(); it != end;) {
        auto index = *it;
        TIRO_DEBUG_ASSERT(
            is_allocated_block_start(index), "invalid object block in finalizers table");

        auto pos = it++;
        if (!is_cell_marked(index)) {
            HeapValue value(reinterpret_cast<Header*>(cell(index)));
            finalize(value);
            finalizers_.erase(pos);
        }
    }

    // Optimized sweep that runs through the block & mark bitmaps using efficient block operations.
    //
    // The state before sweeping (after tracing):
    //
    // | B | M | Meaning
    // | - | - | -------
    // | 1 | 0 | dead block
    // | 1 | 1 | live block
    // | 0 | 0 | block extent
    // | 0 | 1 | free block block (first cell in block)
    //
    // The needed transitions are:
    //   10 -> 01   (note: coalesce free blocks to 00 if the previous is also free)
    //   11 -> 10
    //   00 -> 00
    //   01 -> 01   (note: coalesce free blocks to 00 if the previous is also free)
    // which can all be implemented using '&' and '^', see below.
    //
    // Source: http://wiki.luajit.org/New-Garbage-Collector#sweep-phase_bitmap-tricks
    {
        auto block = block_bitmap_storage();
        auto mark = mark_bitmap_storage();
        TIRO_DEBUG_ASSERT(block.size() == mark.size(), "bitmaps must have the same size");
        for (size_t i = 0, n = block.size(); i < n; ++i) {
            auto new_block = block[i] & mark[i];
            auto new_mark = block[i] ^ mark[i];
            mark[i] = new_mark;
            block[i] = new_block;
        }
    }

    // Rebuild the free list.
    // Every '1' in the mark bitmap indicates a free block.
    // We add all free blocks to the free list while ensuring that adjacent free blocks are merged.
    // When iterating over the individual free blocks (and their initial '1' mark bit), we
    // either leave them as-is reset the mark bit to 0 if the previous block is free as well.
    //
    // This step could probably be merged into the last loop for even
    // better cache efficiency, with some additional smarts? Might not be worth it, however ..
    size_t free_cells = 0;
    {
        auto block = block_bitmap();
        auto mark = mark_bitmap();
        TIRO_DEBUG_ASSERT(block.size() == mark.size(), "bitmaps must have the same size");

        const size_t total_cells = cells_count();
        size_t current_free = mark.find_set();
        while (current_free != mark.npos) {
            // all cells until the next live block (or the end of the page) are free.
            const size_t next_live = block.find_set(current_free);
            const size_t free_size = next_live != block.npos ? next_live - current_free
                                                             : total_cells - current_free;

            // register the coalesced block with the free space.
            free_space.insert_free(cells().subspan(current_free, free_size));
            free_cells += free_size;

            // clear the mark bit for free blocks that follow the initial free block.
            // this coalesces them with their predecessor as far as the bitmaps are concerned.
            size_t cursor = current_free + 1;
            while (1) {
                size_t free = mark.find_set(cursor);
                if (free > next_live || free == mark.npos) {
                    current_free = free;
                    break;
                }

                mark.clear(free);
            }
        }
    }
    TIRO_DEBUG_ASSERT(free_cells <= cells_count(), "free count is too large");
    stats.free_cells = free_cells;
    stats.allocated_cells = cells_count() - free_cells;
}

Span<Cell> Page::cells() {
    return Span(cell(0), cells_count());
}

Cell* Page::cell(u32 index) {
    TIRO_DEBUG_ASSERT(index < cells_count(), "cell index out of bounds");
    auto& layout = this->layout();
    char* self = reinterpret_cast<char*>(this);
    Cell* cells = reinterpret_cast<Cell*>(self + layout.cells_offset);
    return cells + index;
}

u32 Page::cell_index(const void* address) {
    TIRO_DEBUG_ASSERT(address, "invalid address");
    size_t page_offset = reinterpret_cast<const char*>(address) - reinterpret_cast<char*>(this);
    size_t index = (page_offset - layout().cells_offset) / cell_size;
    TIRO_DEBUG_ASSERT(index < cells_count(), "cell index out of bounds");
    return static_cast<u32>(index);
}

bool Page::is_cell_marked(u32 index) {
    TIRO_DEBUG_ASSERT(index < cells_count(), "cell index out of bounds");
    TIRO_DEBUG_ASSERT(
        is_allocated_block_start(index), "cell must be the start of an allocated block");
    return mark_bitmap().test(index);
}

void Page::set_cell_marked(u32 index, bool marked) {
    TIRO_DEBUG_ASSERT(index < cells_count(), "cell index out of bounds");
    TIRO_DEBUG_ASSERT(
        is_allocated_block_start(index), "cell must be the start of an allocated block");
    mark_bitmap().set(index, marked);
}

bool Page::is_allocated_block_start(u32 index) {
    TIRO_DEBUG_ASSERT(index < cells_count(), "cell index out of bounds");
    return block_bitmap().test(index);
}

bool Page::is_free_block_start(u32 index) {
    TIRO_DEBUG_ASSERT(index < cells_count(), "cell index out of bounds");
    return mark_bitmap().test(index) && !block_bitmap().test(index);
}

bool Page::is_cell_block_extent(u32 index) {
    TIRO_DEBUG_ASSERT(index < cells_count(), "cell index out of bounds");
    return !mark_bitmap().test(index) && !block_bitmap().test(index);
}

void Page::set_allocated(u32 index, u32 size) {
    TIRO_DEBUG_ASSERT(index <= cells_count(), "cell index out of bounds");
    TIRO_DEBUG_ASSERT(size <= cells_count() - index, "cell range out of bounds");
    TIRO_DEBUG_ASSERT(size > 0, "zero sized cell range");
    // block 1 mark 0 is the start code for free blocks (see table in heap.md)
    block_bitmap().set(index, false);
    mark_bitmap().set(index, false);
    TIRO_DEBUG_ASSERT(get_block_extent(index) >= size, "invalid number of block extent cells");
}

void Page::set_free(u32 index, [[maybe_unused]] u32 size) {
    TIRO_DEBUG_ASSERT(index <= cells_count(), "cell index out of bounds");
    TIRO_DEBUG_ASSERT(size <= cells_count() - index, "cell range out of bounds");
    TIRO_DEBUG_ASSERT(size > 0, "zero sized cell range");
    // block 0 mark 1 is the start code for free blocks (see table in heap.md)
    block_bitmap().set(index, false);
    mark_bitmap().set(index, true);
    TIRO_DEBUG_ASSERT(get_block_extent(index) >= size, "invalid number of block extent cells");
}

void Page::set_finalizer(u32 index) {
    TIRO_DEBUG_ASSERT(index < cells_count(), "cell index out of bounds");
    TIRO_DEBUG_ASSERT(is_allocated_block_start(index), "cell index is not the start of a block");
    finalizers_.insert(index);
}

u32 Page::get_block_extent(u32 index) {
    TIRO_DEBUG_ASSERT(index < cells_count(), "cell index out of bounds");

    auto mark = mark_bitmap();
    auto block = block_bitmap();
    const u32 count = cells_count();
    u32 i = index + 1;
    for (; i < count; ++i) {
        if (mark.test(i) || block.test(i))
            break;
    }
    return i - index;
}

const PageLayout& Page::layout() const {
    return heap().layout();
}

Page::Page(Heap& heap)
    : Chunk(ChunkType::Page, heap) {
    auto blocks = block_bitmap_storage();
    std::uninitialized_fill(blocks.begin(), blocks.end(), 0);

    auto marks = mark_bitmap_storage();
    std::uninitialized_fill(marks.begin(), marks.end(), 0);
}

Page::~Page() {}

Span<Page::BitsetItem> Page::block_bitmap_storage() {
    auto& layout = this->layout();
    char* self = reinterpret_cast<char*>(this);
    BitsetItem* items = reinterpret_cast<BitsetItem*>(self + layout.block_bitmap_offset);
    return Span(items, layout.bitmap_items);
}

Span<Page::BitsetItem> Page::mark_bitmap_storage() {
    auto& layout = this->layout();
    char* self = reinterpret_cast<char*>(this);
    BitsetItem* items = reinterpret_cast<BitsetItem*>(self + layout.mark_bitmap_offset);
    return Span(items, layout.bitmap_items);
}

NotNull<LargeObject*> LargeObject::from_address(const void* address) {
    TIRO_DEBUG_ASSERT(address, "invalid address");
    auto lob = reinterpret_cast<const LargeObject*>(
        reinterpret_cast<const char*>(address) - sizeof(LargeObject));
    return TIRO_NN(const_cast<LargeObject*>(lob));
}

NotNull<LargeObject*> LargeObject::allocate(Heap& heap, u32 cells) {
    TIRO_DEBUG_ASSERT(cells > 0, "zero sized allocation");

    void* block = heap.allocate_raw(dynamic_size(cells), cell_align);
    if (!block)
        TIRO_ERROR("failed to allocate large object chunk");

    return TIRO_NN(new (block) LargeObject(heap, cells));
}

size_t LargeObject::dynamic_size(u32 cells) {
    return sizeof(LargeObject) + cells * sizeof(Cell);
}

void LargeObject::destroy(NotNull<LargeObject*> lob) {
    static_assert(std::is_trivially_destructible_v<LargeObject>);
    auto& heap = lob->heap();
    heap.free_raw(static_cast<void*>(lob.get()),
        sizeof(LargeObject) + lob->cells_count() * sizeof(Cell), cell_align);
}

Span<Cell> LargeObject::cells() {
    Cell* data = reinterpret_cast<Cell*>(this + 1);
    return Span(data, cells_count_);
}

bool LargeObject::is_marked() {
    return marked_;
}

void LargeObject::set_marked(bool value) {
    marked_ = value;
}

FreeSpace::FreeSpace(const PageLayout& layout)
    : layout_(layout) {
    const u32 largest_class_size = ceil_pow2_fast(layout.cells_size) >> 2;
    TIRO_DEBUG_ASSERT(largest_class_size >= first_exp_size_class, "invalid cells per page value");
    exp_size_classes_ = (log2_fast(largest_class_size) - first_exp_size_class_log) * 2;
    lists_.resize(class_count());
}

Cell* FreeSpace::allocate_exact(u32 request) {
    TIRO_DEBUG_ASSERT(request > 0, "zero sized allocation");
    TIRO_TRACE_FREE_SPACE("attempting to allocate {} cells\n", request);

    const u32 classes = lists_.size();
    for (u32 index = class_index(request); index < classes; ++index) {
        TIRO_TRACE_FREE_SPACE("searching size class {} (>= {})\n", index, class_size(index));

        FreeList& list = lists_[index];
        Span<Cell> result = first_fit(list, request);
        if (result.empty())
            continue;

        TIRO_DEBUG_ASSERT(result.size() >= request, "first fit did not return a valid result");

        // Mark cells as allocated block in page metadata.
        Cell* cell = result.data();
        NotNull<Page*> page = Page::from_address(result.data(), layout_);
        page->set_allocated(page->cell_index(cell), request);

        // Split remaining free bytes at the end into a new free block, if necessary.
        if (result.size() > request) {
            TIRO_TRACE_FREE_SPACE(
                "allocated match {} of size {}\n", (void*) result.data(), result.size());

            // Mark cells as free block in page metadata since the found block was split.
            auto free_cells = result.drop_front(request);
            insert_free_with_metadata(free_cells);
        } else {
            TIRO_TRACE_FREE_SPACE("allocated exact match {}\n", (void*) result.data());
        }
        return cell;
    }

    // No match
    TIRO_TRACE_FREE_SPACE("allocation failed\n");
    return nullptr;
}

Span<Cell> FreeSpace::allocate_chunk(u32 request) {
    TIRO_DEBUG_ASSERT(request > 0, "zero sized allocation");
    TIRO_TRACE_FREE_SPACE("attempting to allocate {} or more cells\n", request);

    // Reverse search through buckets to favor larger chunks
    const u32 classes = lists_.size();
    const u32 min_class = class_index(request);
    for (u32 index = classes; index-- > min_class;) {
        TIRO_TRACE_FREE_SPACE("searching size class {} (>= {})\n", index, class_size(index));

        FreeList& list = lists_[index];
        Span<Cell> result = first_fit(list, request);
        if (result.empty())
            continue;

        TIRO_DEBUG_ASSERT(result.size() >= request, "first fit did not return a valid result");
        TIRO_TRACE_FREE_SPACE(
            "allocated match {} of size {}\n", (void*) result.data(), result.size());

        // Mark cells as allocated block in page metadata.
        Cell* cell = result.data();
        NotNull<Page*> page = Page::from_address(cell, layout_);
        page->set_allocated(page->cell_index(cell), result.size());
        return result;
    }

    // No match
    TIRO_TRACE_FREE_SPACE("allocation failed\n");
    return {};
}

void FreeSpace::insert_free(Span<Cell> cells) {
    TIRO_DEBUG_ASSERT(cells.size() > 0, "zero sized free");

#ifdef TIRO_DEBUG
    auto page = Page::from_address(cells.data(), layout_);
    TIRO_DEBUG_ASSERT(page->is_free_block_start(page->cell_index(cells.data())),
        "blocks on the free list must be marked as free within the containing page");
#endif

    const u32 index = class_index(cells.size());
    TIRO_TRACE_FREE_SPACE("freeing {} ({} cells) by pushing into list {} (>= {})\n",
        (void*) cells.data(), cells.size(), index, class_size(index));

    FreeList& list = lists_[index];
    push(list, cells);
}

void FreeSpace::insert_free_with_metadata(Span<Cell> cells) {
    TIRO_DEBUG_ASSERT(cells.size() > 0, "zero sized free");

    auto page = Page::from_address(cells.data(), layout_);
    page->set_free(page->cell_index(cells.data()), cells.size());
    insert_free(cells);
}

void FreeSpace::reset() {
    std::fill(lists_.begin(), lists_.end(), FreeList());
}

u32 FreeSpace::class_index(u32 alloc) const {
    TIRO_DEBUG_ASSERT(alloc > 0, "zero sized allocation");
    if (alloc <= exact_size_classes) {
        return alloc - 1; // size class 0 has cell size 1
    }

    const u32 log = log2_fast(alloc);
    u32 index = (log - first_exp_size_class_log) * 2;
    if (alloc - (u32(1) << log) >= (u32(1) << (log - 1)))
        ++index;
    return std::min(index + exact_size_classes, class_count() - 1);
}

u32 FreeSpace::class_size(u32 index) const {
    TIRO_DEBUG_ASSERT(index < class_count(), "invalid size class index");
    if (index < exact_size_classes)
        return index + 1; // size class 0 has cell size 1

    const u32 exp_index = first_exp_size_class_log + ((index - exact_size_classes) >> 1);
    return (1 << exp_index) | ((1 & (index + 1)) << (exp_index - 1));
}

u32 FreeSpace::class_count() const {
    return exact_size_classes + exp_size_classes_ + 1;
}

Span<Cell> FreeSpace::first_fit(FreeList& list, u32 request) {
    FreeListEntry** cursor = &list.head;
    while (*cursor) {
        FreeListEntry* entry = *cursor;
        if (entry->cells >= request) {
            *cursor = entry->next;
            return Span(reinterpret_cast<Cell*>(entry), entry->cells);
        }
        cursor = &entry->next;
    }
    return {};
}

void FreeSpace::push(FreeList& list, Span<Cell> cells) {
    TIRO_DEBUG_ASSERT(cells.size() > 0, "zero sized cell span");
    static_assert(sizeof(FreeListEntry) <= sizeof(Cell));
    static_assert(alignof(FreeListEntry) <= alignof(Cell));
    list.head = new (cells.data()) FreeListEntry(list.head, cells.size());
}

Span<Cell> FreeSpace::pop(FreeList& list) {
    FreeListEntry* entry = list.head;
    if (!entry)
        return {};

    list.head = entry->next;
    return Span(reinterpret_cast<Cell*>(entry), entry->cells);
}

Heap::Heap(size_t page_size, HeapAllocator& alloc)
    : alloc_(alloc)
    , layout_(Page::compute_layout(page_size))
    , collector_(*this)
    , free_(layout_) {}

Heap::~Heap() {
    // Quick and dirty way to invoke finalizers when the heap is shutting down.
    sweep();

    for (auto p : pages_)
        Page::destroy(p);
    pages_.clear();

    for (auto lob : lobs_)
        LargeObject::destroy(lob);
    lobs_.clear();
}

void* Heap::allocate(size_t bytes_request) {
    TIRO_DEBUG_ASSERT(bytes_request > 0, "zero sized allocation");
    if (TIRO_UNLIKELY(bytes_request > max_allocation_size))
        throw std::bad_alloc();

    bool collector_ran = false;
    if (stats_.allocated_bytes >= collector_.next_threshold()) {
        collector_.collect(GcReason::Automatic);
        collector_ran = true;
    }

    // Objects of large size get an allocation of their own.
    const u32 cells_request = ceil_div(bytes_request, cell_size);
    if (cells_request >= layout_.large_object_cells) {
        auto lob = add_lob(cells_request);
        stats_.allocated_bytes += bytes_request;
        return lob->cells().data();
    }

    // All other objects are allocated from some free storage on a page.
    auto try_allocate = [&]() { return free_.allocate_exact(cells_request); };

    // Initial allocation.
    void* result = try_allocate();

    // Run collector if initial allocation fails.
    if (!result && !collector_ran) {
        collector_.collect(GcReason::AllocFailure);
        collector_ran = true;
        result = try_allocate();
    }

    // Allocate a new page if still no success after gc.
    if (!result) {
        add_page();
        result = try_allocate();
        if (TIRO_UNLIKELY(!result))
            TIRO_ERROR("allocation request failed after new page was allocated");
    }

    stats_.allocated_bytes += bytes_request;
    stats_.free_bytes -= bytes_request;
    return result;
}

void Heap::sweep() {
    // Reset 'allocated' / 'free' counters and recompute them during the sweep.
    // Note that the 'total' counter is not reset.
    stats_.free_bytes = 0;
    stats_.allocated_bytes = 0;
    free_.reset();

    // see absl flat_hash_set::erase for the erase_if idiom.
    for (auto it = lobs_.begin(), end = lobs_.end(); it != end;) {
        auto lob = *it;
        auto erase = it++;
        if (!lob->is_marked()) {
            lobs_.erase(erase);
            destroy_lob(lob);
        } else {
            lob->set_marked(false);
            stats_.allocated_bytes += lob->cells_count() * cell_size;
        }
    }

    for (auto it = pages_.begin(), end = pages_.end(); it != end;) {
        auto page = *it;

        Page::SweepStats page_stats;
        page->sweep(page_stats, free_);
        stats_.allocated_bytes += page_stats.allocated_cells * cell_size;
        stats_.free_bytes += page_stats.free_cells * cell_size;
    }
}

void* Heap::allocate_raw(size_t size, size_t align) {
    TIRO_DEBUG_ASSERT(stats_.total_bytes <= max_size_, "invalid total bytes count");
    if (TIRO_UNLIKELY(size > max_size_ - stats_.total_bytes))
        TIRO_ERROR_WITH_CODE(TIRO_ERROR_ALLOC, "memory limit reached");
    stats_.total_bytes += size;

    void* result = alloc_.allocate_aligned(size, align);
    if (TIRO_UNLIKELY(!result))
        TIRO_ERROR("failed to allocate block of size {}", size);
    return result;
}

void Heap::free_raw(void* block, size_t size, size_t align) {
    TIRO_DEBUG_ASSERT(size <= stats_.total_bytes, "invalid total bytes count");
    stats_.total_bytes -= size;
    alloc_.free_aligned(block, size, align);
}

NotNull<Page*> Heap::add_page() {
    NotNull<Page*> page = Page::allocate(*this); // may throw
    {
        ScopeFailure cleanup_on_error = [&]() { Page::destroy(page); };
        pages_.insert(page); // may throw
    }
    free_.insert_free_with_metadata(page->cells());
    stats_.free_bytes += layout_.cells_size * cell_size;
    return page;
}

NotNull<LargeObject*> Heap::add_lob(u32 cells) {
    NotNull<LargeObject*> lob = LargeObject::allocate(*this, cells); // may throw
    TIRO_DEBUG_ASSERT(lob->cells_count() == cells, "large object has inconsistent number of cells");

    ScopeFailure cleanup_on_error = [&]() { LargeObject::destroy(lob); };
    lobs_.insert(lob); // may throw
    return lob;
}

void Heap::destroy_lob(NotNull<LargeObject*> lob) {
    TIRO_DEBUG_ASSERT(!lobs_.contains(lob), "large object was not unregistered");
    LargeObject::destroy(lob);
}

} // namespace tiro::vm::new_heap
