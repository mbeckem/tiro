#include "vm/heap/heap.hpp"

#include <algorithm>
#include <new>

#include "fmt/format.h"

#include "common/scope_guards.hpp"
#include "vm/heap/allocator.hpp"

namespace tiro::vm {

#if 0
#    define TIRO_TRACE_FREE_SPACE(...) fmt::print("free space: " __VA_ARGS__);
#else
#    define TIRO_TRACE_FREE_SPACE(...)
#endif

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
        NotNull<Page*> page = Page::from_address(cell, layout_);
        const u32 cell_index = page->cell_index(cell);
        page->set_allocated(cell_index, request);

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
        const u32 cell_index = page->cell_index(cell);
        page->set_allocated(cell_index, result.size());
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
    for (auto page : pages_) {
        page->invoke_finalizers();
        Page::destroy(page);
    }
    pages_.clear();

    for (auto lob : lobs_) {
        lob->invoke_finalizer();
        LargeObject::destroy(lob);
    }
    lobs_.clear();
}

std::tuple<void*, ChunkType> Heap::allocate(size_t bytes_request) {
    TIRO_DEBUG_ASSERT(!collector_.running(), "collector must not be running");
    TIRO_DEBUG_ASSERT(bytes_request > 0, "zero sized allocation");
    if (TIRO_UNLIKELY(bytes_request > max_allocation_size))
        TIRO_ERROR_WITH_CODE(
            TIRO_ERROR_ALLOC, "allocation request is too large: {} bytes", bytes_request);

    // TODO: Improve the decision when the collector runs.
    //  - Should also run if the allocation would exceed the max_size
    bool collector_ran = false;
    if (stats_.allocated_bytes >= collector_.next_threshold()) {
        collector_.collect(GcReason::Automatic);
        collector_ran = true;
    }

    // Objects of large size get an allocation of their own.
    const u32 cells_request = ceil_div(bytes_request, cell_size);
    if (cells_request >= layout_.large_object_cells) {
        auto lob = add_lob(cells_request);
        stats_.allocated_objects += 1;
        stats_.allocated_bytes += bytes_request;
        return std::tuple(lob->cells().data(), ChunkType::LargeObject);
    }

    // All other objects are allocated from some free storage on a page.
    auto try_allocate = [&]() { return free_.allocate_exact(cells_request); };

    // Initial allocation.
    void* result = try_allocate();

    // Run collector if initial allocation fails.
    if (!pages_.empty() && !result && !collector_ran) {
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

    stats_.allocated_objects += 1;
    stats_.allocated_bytes += bytes_request;
    stats_.free_bytes -= bytes_request;
    return std::tuple(result, ChunkType::Page);
}

void Heap::mark_finalizer(ChunkType type, void* address) {
    TIRO_DEBUG_ASSERT(address, "invalid address");

    switch (type) {
    case ChunkType::LargeObject:
        LargeObject::from_address(address)->set_finalizer(true);
        break;
    case ChunkType::Page: {
        auto page = Page::from_address(address, layout_);
        page->mark_finalizer(page->cell_index(address));
        break;
    }
    }
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
            lob->invoke_finalizer();
            lobs_.erase(erase);
            destroy_lob(lob);
        } else {
            lob->set_marked(false);
            stats_.allocated_bytes += lob->cells_count() * cell_size;
        }
    }

    for (auto it = pages_.begin(), end = pages_.end(); it != end; ++it) {
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

} // namespace tiro::vm
