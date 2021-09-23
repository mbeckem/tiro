#include <catch2/catch.hpp>

#include "common/scope_guards.hpp"
#include "vm/heap/memory.hpp"
#include "vm/heap/new_heap.hpp"

#include "support/matchers.hpp"

#include <map>
#include <new>

namespace tiro::vm::new_heap::test {

using test_support::exception_matches_code;

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
    for (size_t page_size = Page::min_size_bytes; page_size <= Page::max_size_bytes;
         page_size *= 2) {
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

        REQUIRE(layout.large_object_cells == layout.cells_size / 4);
    }
}

TEST_CASE("object pointers can be mapped to a pointer to their page", "[heap]") {
    static constexpr size_t page_size = 1 << 16;

    DefaultHeapAllocator alloc;
    Heap heap(page_size, alloc);
    PageLayout layout = heap.layout();
    REQUIRE(layout.page_size == page_size);

    auto page = Page::allocate(heap);
    ScopeExit cleanup_page = [&] { Page::destroy(page); };

    auto data = reinterpret_cast<char*>(page.get());
    REQUIRE(Page::from_address(data, layout).get() == page);
    REQUIRE(Page::from_address(data + 1, layout).get() == page);
    REQUIRE(Page::from_address(data + page_size - 1, layout).get() == page);
    REQUIRE(Page::from_address(data + page_size, layout).get() != page);

    REQUIRE(page->cell_index(data + layout.cells_offset) == 0);
    REQUIRE(page->cell_index(data + layout.cells_offset + cell_size - 1) == 0);
    REQUIRE(page->cell_index(data + layout.cells_offset + cell_size) == 1);
}

TEST_CASE("large object pointers can be mapped to a pointer to their chunk", "[heap]") {
    const size_t page_size = 1 << 16;
    const size_t cells = 123;

    DefaultHeapAllocator alloc;
    Heap heap(page_size, alloc);

    auto chunk = LargeObject::allocate(heap, cells);
    ScopeExit cleanup_chunk = [&] { LargeObject::destroy(chunk); };
    REQUIRE(chunk->cells_count() == cells);

    auto data = reinterpret_cast<char*>(chunk->cells().data());
    REQUIRE(LargeObject::from_address(data).get() == chunk);
    REQUIRE(LargeObject::from_address(data + 1).get() != chunk);
    REQUIRE(LargeObject::from_address(data - 1).get() != chunk);
}

TEST_CASE("free space should report correct class sizes", "[heap]") {
    for (size_t page_size = Page::min_size_bytes; page_size <= Page::max_size_bytes;
         page_size *= 2) {
        CAPTURE(page_size);
        const auto layout = Page::compute_layout(page_size);
        const auto cells = layout.cells_size;

        FreeSpace space(layout);
        std::vector<u32> classes;
        for (u32 i = 0, s = space.class_count(); i < s; ++i)
            classes.push_back(space.class_size(i));

        //fmt::print("cells {}, class count: {}, classes: {}\n", cells, classes.size(),
        //    fmt::join(classes, ","));

        // first size classes are exact size
        u32 index = 0;
        while (index < classes.size()) {
            CAPTURE(index);
            const u32 expected = index + 1;
            if (expected * cell_size >= 256)
                break;

            REQUIRE(classes[index] == expected);
            ++index;
        }

        // rest of the size classes are 2^n, 2^n + 2^(n-1), 2^n+1, ...
        REQUIRE(index % 2 == 1);
        u32 pow = 1 << log2(index);
        while (index < classes.size()) {
            CAPTURE(index);
            if (index % 2 == 1) {
                pow <<= 1;
                REQUIRE(classes[index] == pow);
            } else {
                REQUIRE(classes[index] == pow + (pow >> 1));
            }
            ++index;
        }

        // Max size class is 25% of a page
        REQUIRE(classes.size() % 2 == 0);
        REQUIRE(classes.back() == (ceil_pow2(cells) / 4));
    }
}

TEST_CASE("free space should compute the correct class index", "[heap]") {
    for (size_t page_size = Page::min_size_bytes; page_size <= Page::max_size_bytes;
         page_size *= 2) {
        CAPTURE(page_size);
        const auto layout = Page::compute_layout(page_size);
        const auto cells = layout.cells_size;

        FreeSpace space(layout);
        auto validate_class = [&](u32 class_index) {
            u32 class_size = space.class_size(class_index);
            CAPTURE(class_index, class_size);

            // Returns the same index when using the 'start' value, since
            // size class buckets contain buckets >= their associated size.
            u32 exact_match = space.class_index(class_size);
            REQUIRE(exact_match == class_index);

            // Check previous class.
            if (class_index > 0) {
                u32 before = space.class_index(class_size - 1);
                REQUIRE(before == class_index - 1);
            }

            // Check just before start of the next class.
            if (class_index < space.class_count() - 1) {
                u32 next_class_size = space.class_size(class_index + 1);
                u32 last_match = space.class_index(next_class_size - 1);
                REQUIRE(last_match == class_index);
            }
        };

        for (u32 i = 0; i < space.class_count(); ++i) {
            validate_class(i);
        }

        // cells cannot fit, but size index must never go out of bounds.
        u32 large_match = space.class_index(cells);
        REQUIRE(large_match == space.class_count() - 1);
    }
}

TEST_CASE("free space should return freed cell spans", "[heap]") {
    const size_t total_cells = 256;

    DefaultHeapAllocator alloc;
    Heap heap(Page::default_size_bytes, alloc);

    NotNull<Page*> page = Page::allocate(heap);
    ScopeExit cleanup = [&] { Page::destroy(page); };

    Cell* cells = page->cells().data();
    REQUIRE(page->cells().size() >= total_cells);

    // Chunk up the array of cells
    std::map<Cell*, size_t> freed;
    size_t total_freed = 0;
    {
        size_t alloc_size = 1;
        size_t remaining = total_cells;
        Cell* current_cell = cells;
        while (remaining >= alloc_size) {
            freed[current_cell] = alloc_size;
            current_cell += alloc_size;
            total_freed += alloc_size;
            remaining -= alloc_size;

            alloc_size *= 2;
            if (alloc_size > 16)
                alloc_size = 1;
        }
    }

    // Free them
    FreeSpace space(heap.layout());
    for (const auto& entry : freed) {
        space.insert_free_with_metadata(Span(entry.first, entry.second));
    }

    // Allocate them back with 1-sized allocations.
    std::set<Cell*> allocated;
    for (size_t i = 0; i < total_freed; ++i) {
        CAPTURE(i);
        Cell* cell = space.allocate_exact(1);
        REQUIRE(cell);
        REQUIRE(!allocated.count(cell));
        allocated.insert(cell);
    }

    // All cells must have been returned
    REQUIRE(allocated.size() == total_freed);
    for (size_t i = 0; i < total_freed; ++i) {
        CAPTURE(i);
        REQUIRE(allocated.count(cells + i));
    }

    // Further allocations fail
    REQUIRE(space.allocate_exact(1) == nullptr);
}

TEST_CASE("allocate_large should favor large chunks", "[heap]") {
    const size_t total_cells = 256;

    DefaultHeapAllocator alloc;
    Heap heap(Page::default_size_bytes, alloc);

    NotNull<Page*> page = Page::allocate(heap);
    ScopeExit cleanup = [&] { Page::destroy(page); };

    Cell* cells = page->cells().data();
    REQUIRE(page->cells().size() >= total_cells);

    FreeSpace space(heap.layout());
    space.insert_free_with_metadata(Span(cells + 40, 128));
    space.insert_free_with_metadata(Span(cells + 8, 32));
    space.insert_free_with_metadata(Span(cells + 0, 8));

    SECTION("large chunk is returned for large request") {
        auto chunk_a = space.allocate_chunk(120);
        REQUIRE(chunk_a.data() == cells + 40);
        REQUIRE(chunk_a.size() == 128);

        auto chunk_b = space.allocate_chunk(120);
        REQUIRE(chunk_b.empty());

        auto chunk_c = space.allocate_chunk(32);
        REQUIRE(chunk_c.data() == cells + 8);
        REQUIRE(chunk_c.size() == 32);
    }

    SECTION("large chunk is returned for smaller request") {
        // size class of 128 is much larger, therefore its seen first
        auto chunk = space.allocate_chunk(1);
        REQUIRE(chunk.data() == cells + 40);
        REQUIRE(chunk.size() == 128);
    }
}

TEST_CASE("heap should track of total allocated memory", "[heap]") {
    const size_t page_size = 1 << 16;
    DefaultHeapAllocator alloc;

    Heap heap(page_size, alloc);
    REQUIRE(heap.stats().total_bytes == 0);

    const auto page = Page::allocate(heap);
    const auto size_after_page = heap.stats().total_bytes;
    REQUIRE(size_after_page == page_size);

    const auto lob = LargeObject::allocate(heap, 123);
    const auto size_after_lob = heap.stats().total_bytes;
    REQUIRE(size_after_lob >= size_after_page + cell_size * 123); // lobs have some overhead

    Page::destroy(page);
    REQUIRE(heap.stats().total_bytes == size_after_lob - page_size);

    LargeObject::destroy(lob);
    REQUIRE(heap.stats().total_bytes == 0);
}

TEST_CASE("heap should throw when the memory limit has been reached", "[heap]") {
    const size_t page_size = 1 << 16;
    DefaultHeapAllocator alloc;

    Heap heap(page_size, alloc);
    REQUIRE(heap.max_size() == size_t(-1)); // Unlimited by default
    heap.max_size(page_size + 1);

    auto page = Page::allocate(heap);
    ScopeExit cleanup = [&] { Page::destroy(page); };

    REQUIRE_THROWS_MATCHES(
        LargeObject::allocate(heap, 1), tiro::Error, exception_matches_code(TIRO_ERROR_ALLOC));
}

} // namespace tiro::vm::new_heap::test
