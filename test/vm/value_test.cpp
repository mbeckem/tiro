#include <catch.hpp>

#include "tiro/vm/objects/value.hpp"

using namespace tiro::vm;

TEST_CASE("Nullpointer representation should be an actual 0", "[value]") {
    REQUIRE(reinterpret_cast<uintptr_t>(nullptr) == 0);
}
