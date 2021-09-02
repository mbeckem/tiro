#include "vm/heap/new_heap.hpp"

#include <algorithm>
#include <new>

#include "fmt/format.h"

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
    if (page_size < Page::min_size || page_size > Page::max_size)
        TIRO_ERROR("page size must be in the range [{}, {}]: {}", Page::min_size, Page::max_size,
            page_size);

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
    return layout;
}

Span<Page::BitsetItem> Page::block_bitmap() {
    auto& layout = this->layout();
    char* self = reinterpret_cast<char*>(this);
    BitsetItem* items = reinterpret_cast<BitsetItem*>(self + layout.block_bitmap_offset);
    return Span(items, layout.bitmap_items);
}

Span<Page::BitsetItem> Page::mark_bitmap() {
    auto& layout = this->layout();
    char* self = reinterpret_cast<char*>(this);
    BitsetItem* items = reinterpret_cast<BitsetItem*>(self + layout.mark_bitmap_offset);
    return Span(items, layout.bitmap_items);
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
    return (page_offset - layout().cells_offset) / cell_size;
}

const PageLayout& Page::layout() const {
    return heap().layout();
}

FreeSpace::FreeSpace(u32 cells_per_page) {
    const u32 largest_class_size = ceil_pow2_fast(cells_per_page) >> 2;
    TIRO_DEBUG_ASSERT(largest_class_size >= first_exp_size_class, "invalid cells per page value");
    exp_size_classes_ = (log2_fast(largest_class_size) - first_exp_size_class_log) * 2;
    lists_.resize(class_count());
}

Cell* FreeSpace::allocate_exact(u32 request) {
    TIRO_DEBUG_ASSERT(request >= 1, "zero sized allocation");
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
        return result.data();
    }

    // No match
    TIRO_TRACE_FREE_SPACE("allocation failed\n");
    return nullptr;
}

void FreeSpace::free(Span<Cell> cells) {
    TIRO_DEBUG_ASSERT(cells.size() > 0, "zero sized free");

    const u32 index = class_index(cells.size());
    TIRO_TRACE_FREE_SPACE("freeing {} ({} cells) by pushing into list {} (>= {})\n",
        (void*) cells.data(), cells.size(), index, class_size(index));

    FreeList& list = lists_[index];
    push(list, cells);
}

void FreeSpace::reset() {
    std::fill(lists_.begin(), lists_.end(), FreeList());
}

u32 FreeSpace::class_index(u32 alloc) const {
    TIRO_DEBUG_ASSERT(alloc >= 1, "zero sized allocation");
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
    TIRO_DEBUG_ASSERT(cells.size() >= 1, "zero sized cell span");
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
    , layout_(Page::compute_layout(page_size)) {}

Heap::~Heap() {
    for (auto p : pages_)
        destroy_page(p);
}

NotNull<Page*> Heap::create_page() {
    void* block = alloc_.allocate_aligned(layout_.page_size, layout_.page_size);
    if (!block)
        TIRO_ERROR("failed to allocate page");

    return TIRO_NN(new (block) Page(*this));
}

void Heap::destroy_page(NotNull<Page*> page) {
    static_assert(std::is_trivially_destructible_v<Page>);
    alloc_.free_aligned(static_cast<void*>(page.get()), layout_.page_size, layout_.page_size);
}

} // namespace tiro::vm::new_heap
