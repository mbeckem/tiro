#include <catch2/catch.hpp>

#include "common/memory/arena.hpp"

using namespace tiro;

TEST_CASE("Arena allocation should work", "[arena]") {
    Arena a;

    void* a1 = a.allocate(1);
    REQUIRE(is_aligned(uintptr_t(a1), alignof(max_align_t)));

    void* a2 = a.allocate(1);
    REQUIRE(is_aligned(uintptr_t(a2), alignof(max_align_t)));
    REQUIRE(a2 != a1);

    void* a3 = a.allocate(256, 1);
    REQUIRE((byte*) a3 == ((byte*) a2 + 1));

    REQUIRE(a.used_bytes() == 258);
    REQUIRE(a.total_bytes() >= 258);

    void* a4 = a.allocate(a.min_block_size() * 4);
    REQUIRE(is_aligned(uintptr_t(a4), alignof(max_align_t)));
    REQUIRE(a.total_bytes() == a.min_block_size() * 6);

    a.deallocate();
    REQUIRE(a.used_bytes() == 0);
    REQUIRE(a.total_bytes() == 0);
}
