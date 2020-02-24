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
