#include <catch2/catch.hpp>

#include "vm/heap/new_heap.hpp"

#include <new>

namespace tiro::vm::new_heap::test {

namespace {

struct DummyAllocator : HeapAllocator {
    virtual void*
    allocate_aligned([[maybe_unused]] size_t size, [[maybe_unused]] size_t align) override {
        TIRO_NOT_IMPLEMENTED();
    }

    virtual void free_aligned([[maybe_unused]] void* block, [[maybe_unused]] size_t size,
        [[maybe_unused]] size_t align) override {
        TIRO_NOT_IMPLEMENTED();
    }
};

} // namespace

TEST_CASE("heap constants should contain valid values", "[heap]") {
    STATIC_REQUIRE(is_pow2(object_align));
    STATIC_REQUIRE(object_align_bits >= 2);
}

TEST_CASE("heap facts should be computed correctly", "[heap]") {
    HeapFacts facts(8);
    REQUIRE(facts.page_size == 8);
    REQUIRE(facts.page_size_log == 3);
    REQUIRE(facts.page_mask == static_cast<uintptr_t>(~7));

    REQUIRE((1 & facts.page_mask) == 0);
    REQUIRE((8 & facts.page_mask) == 8);
    REQUIRE((11 & facts.page_mask) == 8);
    REQUIRE((15 & facts.page_mask) == 8);
    REQUIRE((16 & facts.page_mask) == 16);
    REQUIRE((8001 & facts.page_mask) == 8000);
}

TEST_CASE("heap facts should throw when the page size is invalid", "[heap]") {
    REQUIRE_THROWS_AS(HeapFacts(1 + (1 << 16)), Error); /* not a power of two */
}

TEST_CASE("object pointers can be mapped to a pointer to their page", "[heap]") {
    DummyAllocator alloc;
    Heap heap(256, alloc);

    union alignas(256) {
        byte data[256];
        Page p;
    } page_storage{};

    Page* page = new (&page_storage.p) Page(heap);
    REQUIRE(reinterpret_cast<byte*>(page) == &page_storage.data[0]);
    REQUIRE(Page::from_address(heap, page).get() == page);
    REQUIRE(Page::from_address(heap, &page_storage.data[1]).get() == page);
    REQUIRE(Page::from_address(heap, &page_storage.data[255]).get() == page);
    REQUIRE(Page::from_address(heap, &page_storage.data[256]).get() != page);
}

} // namespace tiro::vm::new_heap::test
