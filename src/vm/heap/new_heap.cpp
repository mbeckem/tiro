#include "vm/heap/new_heap.hpp"

#include <algorithm>
#include <new>

#include "fmt/format.h"

#include "common/scope_guards.hpp"
#include "vm/heap/memory.hpp"

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
    auto& alloc = heap.alloc();
    auto& layout = heap.layout();
    void* block = alloc.allocate_aligned(layout.page_size, layout.page_size);
    if (!block)
        TIRO_ERROR("failed to allocate page");

    return TIRO_NN(new (block) Page(heap));
}

void Page::destroy(NotNull<Page*> page) {
    static_assert(std::is_trivially_destructible_v<Page>);

    auto& alloc = page->heap().alloc();
    auto& layout = page->heap().layout();
    alloc.free_aligned(static_cast<void*>(page.get()), layout.page_size, layout.page_size);
}

BitsetView<Page::BitsetItem> Page::block_bitmap() {
    auto storage = block_bitmap_storage();
    return BitsetView(storage, cells_count());
}

BitsetView<Page::BitsetItem> Page::mark_bitmap() {
    auto storage = mark_bitmap_storage();
    return BitsetView(storage, cells_count());
}

Span<Cell> Page::cells() {
    auto& layout = this->layout();
    char* self = reinterpret_cast<char*>(this);
    Cell* cells = reinterpret_cast<Cell*>(self + layout.cells_offset);
    return Span(cells, layout.cells_size);
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

void Page::clear_marked() {
    mark_bitmap().clear();
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

    void* block = heap.alloc().allocate_aligned(
        sizeof(LargeObject) + cells * sizeof(Cell), cell_align);
    if (!block)
        TIRO_ERROR("failed to allocate large object chunk");

    return TIRO_NN(new (block) LargeObject(heap, cells));
}

void LargeObject::destroy(NotNull<LargeObject*> lob) {
    static_assert(std::is_trivially_destructible_v<LargeObject>);
    auto& alloc = lob->heap().alloc();
    alloc.free_aligned(static_cast<void*>(lob.get()),
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
        if (result.size() > request) {
            TIRO_TRACE_FREE_SPACE(
                "allocated match {} of size {}\n", (void*) result.data(), result.size());
            free(result.drop_front(request));
        } else {
            TIRO_TRACE_FREE_SPACE("allocated exact match {}\n", (void*) result.data());
        }

        // Mark cells as allocated block in page metadata.
        Cell* cell = result.data();
        NotNull<Page*> page = Page::from_address(cell, layout_);
        page->set_allocated(page->cell_index(cell), request);
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

void FreeSpace::free(Span<Cell> cells) {
    TIRO_DEBUG_ASSERT(cells.size() > 0, "zero sized free");

    const u32 index = class_index(cells.size());
    TIRO_TRACE_FREE_SPACE("freeing {} ({} cells) by pushing into list {} (>= {})\n",
        (void*) cells.data(), cells.size(), index, class_size(index));

    FreeList& list = lists_[index];
    push(list, cells);

    // Mark cells as free block in page metadata.
    Cell* cell = cells.data();
    NotNull<Page*> page = Page::from_address(cell, layout_);
    page->set_free(page->cell_index(cell), cells.size());
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
    if (allocated_bytes_ >= collector_.next_threshold()) {
        collector_.collect(GcReason::Automatic);
        collector_ran = true;
    }

    // Objects of large size get an allocation of their own.
    const u32 cells_request = ceil_div(bytes_request, cell_size);
    if (cells_request >= layout_.large_object_cells) {
        NotNull<LargeObject*> lob = LargeObject::allocate(*this, cells_request); // may throw
        ScopeFailure cleanup_on_error = [&]() { LargeObject::destroy(lob); };
        TIRO_DEBUG_ASSERT(
            lob->cells_count() == cells_request, "large object has inconsistent number of cells");
        lobs_.insert(lob); // may throw
        allocated_bytes_ += bytes_request;
        return lob->cells().data();
    }

    // All other objects are allocated from some free storage on a page.
    auto try_allocate = [&]() { return free_.allocate_exact(cells_request); };
    void* result = try_allocate();
    if (!result && !collector_ran) {
        collector_.collect(GcReason::AllocFailure);
        collector_ran = true;
        result = try_allocate();
    }
    if (!result) {
        NotNull<Page*> fresh_page = Page::allocate(*this); // may throw
        {
            ScopeFailure cleanup_on_error = [&]() { Page::destroy(fresh_page); };
            pages_.insert(fresh_page); // may throw
        }

        // TODO: Requires more metadata to be set in the page once the bitmaps are in use.
        free_.free(fresh_page->cells());
        result = free_.allocate_exact(cells_request);
        if (TIRO_UNLIKELY(!result))
            TIRO_ERROR("allocation request failed after new page was allocated");
    }

    allocated_bytes_ += bytes_request;
    return result;
}

void Heap::clear_marked() {
    for (auto p : pages_)
        p->clear_marked();

    for (auto lob : lobs_)
        lob->set_marked(false);
}

} // namespace tiro::vm::new_heap
