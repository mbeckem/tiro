#include <catch2/catch.hpp>

#include "common/scope_guards.hpp"
#include "vm/heap/memory.hpp"
#include "vm/heap/new_heap.hpp"

#include <new>

namespace tiro::vm::new_heap::test {

TEST_CASE("heap constants should contain valid values", "[heap]") {
    STATIC_REQUIRE(is_pow2(cell_align));
    STATIC_REQUIRE(cell_align_bits >= 2);
}

TEST_CASE("page mask should be computed correctly", "[heap]") {
    static constexpr size_t page_size = 1 << 16;

    PageLayout layout = Page::compute_layout(page_size);
    REQUIRE(layout.page_size == page_size);
    REQUIRE(layout.page_size_log == 16);
    REQUIRE(layout.page_mask() == static_cast<uintptr_t>(~65535));

    REQUIRE((1 & layout.page_mask()) == 0);
    REQUIRE((123 & layout.page_mask()) == 0);
    REQUIRE((65535 & layout.page_mask()) == 0);
    REQUIRE((65536 & layout.page_mask()) == 65536);
    REQUIRE((3604603 & layout.page_mask()) == 3604480);
}

TEST_CASE("page layout should throw when the page size is invalid", "[heap]") {
    REQUIRE_THROWS_AS(Page::compute_layout(1 + (1 << 16)), Error); /* not a power of two */
}

TEST_CASE("computed page layout should be correct", "[heap]") {
    for (size_t page_size = Page::min_size; page_size <= Page::max_size; page_size *= 2) {
        CAPTURE(page_size);
        const PageLayout layout = Page::compute_layout(page_size);

        // Block bitmap must start after page with correct alignment.
        REQUIRE(layout.block_bitmap_offset == sizeof(Page));
        REQUIRE(layout.block_bitmap_offset % alignof(Page::BitsetItem) == 0);
        REQUIRE(layout.block_bitmap_offset % cell_align == 0);

        // Mark bitmap must follow immediately.
        REQUIRE(layout.mark_bitmap_offset
                == layout.block_bitmap_offset + layout.bitmap_items * sizeof(Page::BitsetItem));
        REQUIRE(layout.mark_bitmap_offset % alignof(Page::BitsetItem) == 0);
        REQUIRE(layout.mark_bitmap_offset % cell_align == 0);

        // Bitmaps must have enough bits for all cells.
        const size_t bits_in_bitset = layout.bitmap_items * type_bits<Page::BitsetItem>();
        REQUIRE((layout.bitmap_items * sizeof(Page::BitsetItem)) % cell_size == 0);
        REQUIRE(bits_in_bitset >= layout.cells_size);

        // Cells must start after the bitmaps, with correct alignment.
        REQUIRE(layout.cells_offset
                == layout.mark_bitmap_offset + layout.bitmap_items * sizeof(Page::BitsetItem));
        REQUIRE(layout.cells_offset % cell_align == 0);
        REQUIRE(layout.cells_offset + layout.cells_size * cell_size <= page_size);

        // Only very little space is wasted at the end of the page.
        const size_t wasted = page_size - (layout.cells_offset + layout.cells_size * cell_size);
        REQUIRE(wasted <= 2 * cell_size);
    }
}

TEST_CASE("object pointers can be mapped to a pointer to their page", "[heap]") {
    static constexpr size_t page_size = 1 << 16;

    DefaultHeapAllocator alloc;
    Heap heap(page_size, alloc);
    PageLayout layout = heap.layout();

    char* data = reinterpret_cast<char*>(alloc.allocate_aligned(page_size, page_size));
    ScopeExit cleanup_page = [&] { alloc.free_aligned(data, page_size, page_size); };
    auto page = TIRO_NN(new (data) Page(heap));

    REQUIRE(Page::from_address(data, layout).get() == page);
    REQUIRE(Page::from_address(data + 1, layout).get() == page);
    REQUIRE(Page::from_address(data + page_size - 1, layout).get() == page);
    REQUIRE(Page::from_address(data + page_size, layout).get() != page);

    REQUIRE(page->cell_index(data + layout.cells_offset) == 0);
    REQUIRE(page->cell_index(data + layout.cells_offset + cell_size - 1) == 0);
    REQUIRE(page->cell_index(data + layout.cells_offset + cell_size) == 1);
}

} // namespace tiro::vm::new_heap::test
