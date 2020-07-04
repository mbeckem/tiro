#include <catch.hpp>

#include "common/hash.hpp"

using namespace tiro;

// TODO: Do some analysis regarding hash collisions, i dont know whether the
// hash combine algorithm in the Hasher class is actually good.

TEST_CASE("Tuples are hashable", "[hash]") {
    UseHasher h;
    size_t hash = h(std::tuple(6, std::string_view("Hello World")));
    REQUIRE(hash != 0);
}
