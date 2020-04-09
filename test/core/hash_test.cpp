#include <catch.hpp>

#include "tiro/core/hash.hpp"

using namespace tiro;

// TODO: Do some analysis regarding hash collisions, i dont know whether the
// hash combine algorithm in the Hasher class is actually good.

TEST_CASE("Tuples are hashable", "[hash]") {
    Hasher h;
    h.append(std::tuple(6, std::string_view("Hello World")));
    h.hash();
}

TEST_CASE("Hashing should modify the running hash value", "[hash]") {
    Hasher h;

    const auto h1 = h.hash();
    h.append(123);

    const auto h2 = h.hash();
    h.append(std::string("foo"));

    const auto h3 = h.hash();

    REQUIRE(h1 != h2);
    REQUIRE(h2 != h3);
}
