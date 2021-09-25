#include "vm/heap/chunks.hpp"

#include "vm/heap/heap.hpp"
#include "vm/objects/value.hpp"

namespace tiro::vm {

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
    invoke_finalizers();

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

void Page::invoke_finalizers() {
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
    block_bitmap().set(index, true);
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

void Page::mark_finalizer(u32 index) {
    TIRO_DEBUG_ASSERT(index < cells_count(), "cell index out of bounds");
    TIRO_DEBUG_ASSERT(is_allocated_block_start(index), "cell index is not the start of a block");
    TIRO_DEBUG_ASSERT(!finalizers_.contains(index), "cell already marked as having a finalizer");
    finalizers_.insert(index);
}

u32 Page::get_block_extent(u32 index) {
    TIRO_DEBUG_ASSERT(index < cells_count(), "cell index out of bounds");

    auto mark = mark_bitmap();
    auto block = block_bitmap();
    size_t count = cells_count();
    size_t i = index + 1;
    // no limit param for find_set yet :(
    size_t n = std::min({count, mark.find_set(i), block.find_set(i)});
    return n - i + 1;
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
    return Span(cell(), cells_count_);
}

Cell* LargeObject::cell() {
    Cell* data = reinterpret_cast<Cell*>(this + 1);
    return data;
}

bool LargeObject::is_marked() {
    return marked_;
}

void LargeObject::set_marked(bool value) {
    marked_ = value;
}

bool LargeObject::has_finalizer() const {
    return finalizer_;
}

void LargeObject::set_finalizer(bool value) {
    finalizer_ = value;
}

void LargeObject::invoke_finalizer() {
    if (finalizer_) {
        HeapValue value(reinterpret_cast<Header*>(cell()));
        finalize(value);
        finalizer_ = false;
    }
}

} // namespace tiro::vm
