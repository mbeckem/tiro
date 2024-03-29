#include <catch2/catch.hpp>

#include "common/scope_guards.hpp"
#include "vm/heap/memory.hpp"

namespace tiro::vm::test {

TEST_CASE("Container mask should round down to alignment", "[memory]") {
    const uintptr_t mask = aligned_container_mask(16);
    REQUIRE((mask & 16) == 16);
    REQUIRE((mask & 32) == 32);
    REQUIRE((mask & 31) == 16);
}

TEST_CASE("Aligned container access should return parent instance", "[memory]") {
    const size_t align = 32;
    const auto mask = aligned_container_mask(align);

    byte* parent = static_cast<byte*>(allocate_aligned(align, align));
    ScopeExit guard = [&] { deallocate_aligned(parent, align, align); };

    for (size_t i = 0; i < align; ++i) {
        CAPTURE(i);
        if (aligned_container_from_member(parent + i, mask) != parent)
            FAIL("Did not return the parent instance.");
    }

    REQUIRE(aligned_container_from_member(parent + align, mask) != parent);
}

TEST_CASE("Aligned allocation should succeed for large blocks", "[memory]") {
    const size_t sizes[] = {
        1 << 12,
        1 << 16,
        1 << 20,
        1 << 22,
    };

    for (size_t size : sizes) {
        CAPTURE(size);

        void* block = nullptr;
        ScopeExit guard = [&] { deallocate_aligned(block, size, size); };
        REQUIRE_NOTHROW(block = allocate_aligned(size, size));
        REQUIRE(block != nullptr);
    }
}

} // namespace tiro::vm::test
