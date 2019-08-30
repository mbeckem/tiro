#include <catch.hpp>

#include "hammer/core/arena.hpp"

using namespace hammer;

TEST_CASE("arena allocation", "[arena]") {
    Arena a;

    void* a1 = a.allocate(0);
    REQUIRE(a1 == nullptr);

    void* a2 = a.allocate(1);
    REQUIRE(is_aligned(uintptr_t(a2), alignof(max_align_t)));

    void* a3 = a.allocate(1);
    REQUIRE(is_aligned(uintptr_t(a3), alignof(max_align_t)));
    REQUIRE(a3 != a2);

    void* a4 = a.allocate(256, 1);
    REQUIRE((byte*) a4 == ((byte*) a3 + 1));

    REQUIRE(a.used_bytes() == 258);
    REQUIRE(a.total_bytes() >= 258);

    void* a5 = a.allocate(a.min_block_size() * 4);
    REQUIRE(is_aligned(uintptr_t(a5), alignof(max_align_t)));
    REQUIRE(a.total_bytes() == a.min_block_size() * 6);

    a.deallocate();
    REQUIRE(a.used_bytes() == 0);
    REQUIRE(a.total_bytes() == 0);
}
