#include <catch.hpp>

#include "hammer/vm/objects/value.hpp"

using namespace hammer::vm;

TEST_CASE("Nullpointer representation", "[value]") {
    REQUIRE(reinterpret_cast<uintptr_t>(nullptr) == 0);
}
