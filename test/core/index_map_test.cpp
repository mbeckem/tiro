#include <catch.hpp>

#include "tiro/core/id_type.hpp"
#include "tiro/core/index_map.hpp"

using namespace tiro;

namespace {

TIRO_DEFINE_ID(Key, u32)

} // namespace

using Map = IndexMap<int, IdMapper<Key>>;

TEST_CASE("IndexMap should have an empty initial state.", "[index-map]") {
    Map map;
    REQUIRE(map.empty());
    REQUIRE(map.size() == 0);
    REQUIRE(map.capacity() == 0);
    REQUIRE_FALSE(map.in_bounds(Key(0)));
}

TEST_CASE("IndexMap should support insertion", "[index-map]") {
    Map map;
    const Key k1 = map.push_back(123);
    const Key k2 = map.push_back(456);
    const Key k3 = map.push_back(789);
    REQUIRE(!map.empty());
    REQUIRE(map.size() == 3);

    REQUIRE(k1 == Key(0));
    REQUIRE(k2 == Key(1));
    REQUIRE(k3 == Key(2));

    REQUIRE(map[k1] == 123);
    REQUIRE(map[k2] == 456);
    REQUIRE(map[k3] == 789);

    map[k2] *= -1;
    REQUIRE(map[k2] == -456);
}

TEST_CASE("IndexMap should support resize()", "[index-map]") {
    Map map;

    // Larger size: fill with placeholders
    map.resize(123, 999);
    REQUIRE(map.size() == 123);
    REQUIRE(map[Key(0)] == 999);
    REQUIRE(map[Key(122)] == 999);

    // Smaller size: don't alter existing elements
    map.resize(55, 777);
    REQUIRE(map[Key(54)] == 999);
}

TEST_CASE("IndexMap should support reserve()", "[index-map]") {
    Map map;
    map.reserve(555);
    REQUIRE(map.capacity() >= 555);
}

TEST_CASE("IndexMap should support handing out pointers", "[index-map]") {
    Map map;
    const Key k1 = map.push_back(10);
    const Key k2 = map.push_back(20);

    auto p1 = map.ptr_to(k1);
    REQUIRE(*p1 == 10);

    auto p2 = map.ptr_to(k2);
    REQUIRE(*p2 == 20);
}

TEST_CASE("Index map should replace all elements during reset", "[index-map]") {
    Map map;

    const Key k1 = map.push_back(1);
    REQUIRE(map[k1] == 1);

    map.reset(2, -1);
    REQUIRE(map.size() == 2);
    REQUIRE(map[k1] == -1);
}
