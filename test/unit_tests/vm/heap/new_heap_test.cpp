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
    REQUIRE(layout.page_mask == static_cast<uintptr_t>(~65535));

    REQUIRE((1 & layout.page_mask) == 0);
    REQUIRE((123 & layout.page_mask) == 0);
    REQUIRE((65535 & layout.page_mask) == 0);
    REQUIRE((65536 & layout.page_mask) == 65536);
    REQUIRE((3604603 & layout.page_mask) == 3604480);
}

TEST_CASE("page layout should throw when the page size is invalid", "[heap]") {
    REQUIRE_THROWS_AS(Page::compute_layout(1 + (1 << 16)), Error); /* not a power of two */
}

TEST_CASE("computed page layout should be correct", "[heap]") {
    for (size_t page_size = Page::min_size; page_size <= Page::max_size; page_size *= 2) {
        CAPTURE(page_size);
        const PageLayout layout = Page::compute_layout(page_size);

        // Bitset must start after page with correct alignment.
        REQUIRE(layout.bitset_items_offset == sizeof(Page));
        REQUIRE(layout.bitset_items_offset % cell_size == 0);
        REQUIRE(layout.bitset_items_offset % alignof(Page::BitsetItem) == 0);

        // Bitset must have enough bits for all cells.
        const size_t bits_in_bitset = layout.bitset_items_size * type_bits<Page::BitsetItem>();
        REQUIRE(bits_in_bitset >= layout.cells_size);

        // Cells must start after the bitset, with correct alignment.
        REQUIRE(
            layout.cells_offset
            == layout.bitset_items_offset + layout.bitset_items_size * sizeof(Page::BitsetItem));
        REQUIRE(layout.cells_offset % cell_size == 0);
        REQUIRE(layout.cells_offset + layout.cells_size * cell_size <= page_size);

        // Only very little space is wasted at the end of the page.
        const size_t wasted = page_size - (layout.cells_offset + layout.cells_size * cell_size);
        REQUIRE(wasted <= cell_size);
    }
}

TEST_CASE("object pointers can be mapped to a pointer to their page", "[heap]") {
    static constexpr size_t page_size = 1 << 16;

    PageLayout layout = Page::compute_layout(page_size);
    char* data = reinterpret_cast<char*>(allocate_aligned(page_size, page_size));
    ScopeExit cleanup_page = [&] { deallocate_aligned(data, page_size, page_size); };

    REQUIRE(Page::from_address(layout, data).get() == (void*) data);
    REQUIRE(Page::from_address(layout, data + 1).get() == (void*) data);
    REQUIRE(Page::from_address(layout, data + page_size - 1).get() == (void*) data);
    REQUIRE(Page::from_address(layout, data + page_size).get() != (void*) data);
}

} // namespace tiro::vm::new_heap::test
