#include "vm/heap/new_heap.hpp"

#include <new>

#include "vm/heap/memory.hpp"

namespace tiro::vm::new_heap {

HeapAllocator::~HeapAllocator() {}

DefaultHeapAllocator::~DefaultHeapAllocator() {}

void* DefaultHeapAllocator::allocate_aligned(size_t size, size_t align) {
    return tiro::vm::allocate_aligned(size, align);
}

void DefaultHeapAllocator::free_aligned(void* block, size_t size, size_t align) {
    return tiro::vm::deallocate_aligned(block, size, align);
}

NotNull<Page*> Page::from_address(Heap& heap, const void* address) {
    return from_address(heap.layout(), address);
}

NotNull<Page*> Page::from_address(const PageLayout& layout, const void* address) {
    TIRO_DEBUG_ASSERT(address, "invalid address");
    return TIRO_NN(reinterpret_cast<Page*>(
        aligned_container_from_member(const_cast<void*>(address), layout.page_mask)));
}

PageLayout Page::compute_layout(size_t page_size) {
    if (page_size < Page::min_size || page_size > Page::max_size)
        TIRO_ERROR("page size must be in the range [{}, {}]: {}", Page::min_size, Page::max_size,
            page_size);

    if (!is_pow2(page_size))
        TIRO_ERROR("page size must be a power of two: {}", page_size);

    PageLayout layout;
    layout.page_size = page_size;
    layout.page_size_log = log2(page_size);
    layout.page_mask = aligned_container_mask(page_size);

    const size_t P = page_size;
    const size_t H = sizeof(Page);
    const size_t C = cell_size;

    // Original equation, where N is the number of cells:
    //
    //      H  +  [(N + 8*C - 1) / (8*C)] * C  +  N*C  <=  P
    //
    // The number of bits in the bitset is rounded up to a multiple of C for simplicity.
    const size_t N = (8 * (P - H - C) + 1) / (1 + 8 * C);

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

    layout.bitset_items_offset = sizeof(Page);
    layout.bitset_items_size = B / sizeof(BitsetItem);
    layout.cells_offset = layout.bitset_items_offset + B;
    layout.cells_size = N;
    return layout;
}

Span<Page::BitsetItem> Page::bitset() {
    auto& layout = this->layout();
    char* self = reinterpret_cast<char*>(this);
    BitsetItem* items = reinterpret_cast<BitsetItem*>(self + layout.bitset_items_offset);
    return Span(items, layout.bitset_items_size);
}

Span<Cell> Page::cells() {
    auto& layout = this->layout();
    char* self = reinterpret_cast<char*>(this);
    Cell* cells = reinterpret_cast<Cell*>(self + layout.cells_offset);
    return Span(cells, layout.cells_size);
}

const PageLayout& Page::layout() const {
    return heap().layout();
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
