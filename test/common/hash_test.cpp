#include <catch.hpp>

#include "common/hash.hpp"

using namespace tiro;

TEST_CASE("Tuples are hashable", "[hash]") {
    UseHasher h;
    size_t hash = h(std::tuple(6, std::string_view("Hello World")));
    REQUIRE(hash != 0);
}
