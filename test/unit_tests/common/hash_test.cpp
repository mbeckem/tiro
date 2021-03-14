#include <catch2/catch.hpp>

#include "common/hash.hpp"

namespace tiro::test {

TEST_CASE("Tuples are hashable", "[hash]") {
    UseHasher h;
    size_t hash = h(std::tuple(6, std::string_view("Hello World")));
    REQUIRE(hash != 0);
}

} // namespace tiro::test
