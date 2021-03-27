#include <catch2/catch.hpp>

#include "common/entities/entity_id.hpp"
#include "common/entities/entity_storage.hpp"

namespace tiro::test {

namespace {

TIRO_DEFINE_ENTITY_ID(Key, u32)

using Storage = EntityStorage<int, Key>;

} // namespace

TEST_CASE("Entity storage should have an empty initial state.", "[entities]") {
    Storage storage;
    REQUIRE(storage.empty());
    REQUIRE(storage.size() == 0);
    REQUIRE(storage.capacity() == 0);
    REQUIRE_FALSE(storage.in_bounds(Key(0)));
}

TEST_CASE("Entity storage should support insertion", "[entities]") {
    Storage storage;
    const Key k1 = storage.push_back(123);
    const Key k2 = storage.push_back(456);
    const Key k3 = storage.push_back(789);
    REQUIRE(!storage.empty());
    REQUIRE(storage.size() == 3);

    REQUIRE(k1 == Key(0));
    REQUIRE(k2 == Key(1));
    REQUIRE(k3 == Key(2));

    REQUIRE(storage[k1] == 123);
    REQUIRE(storage[k2] == 456);
    REQUIRE(storage[k3] == 789);

    storage[k2] *= -1;
    REQUIRE(storage[k2] == -456);
}

TEST_CASE("Entity storage should support access to the front and back element", "[entities]") {
    Storage storage;

    const Key k1 = storage.push_back(123);
    const Key k2 = storage.push_back(456);

    REQUIRE(storage.front() == 123);
    REQUIRE(storage.back() == 456);

    REQUIRE(storage.front_key() == k1);
    REQUIRE(storage.back_key() == k2);
}

TEST_CASE("Entity storage should support removal at the back", "[entities]") {
    Storage storage;
    storage.push_back(123);
    storage.push_back(456);
    REQUIRE(storage.back() == 456);
    REQUIRE(storage.size() == 2);

    storage.pop_back();
    REQUIRE(storage.back() == 123);
    REQUIRE(storage.size() == 1);

    storage.pop_back();
    REQUIRE(storage.size() == 0);
}

TEST_CASE("Entity storage should support resize()", "[entities]") {
    Storage storage;

    // Larger size: fill with placeholders
    storage.resize(123, 999);
    REQUIRE(storage.size() == 123);
    REQUIRE(storage[Key(0)] == 999);
    REQUIRE(storage[Key(122)] == 999);

    // Smaller size: don't alter existing elements
    storage.resize(55, 777);
    REQUIRE(storage[Key(54)] == 999);
}

TEST_CASE("Entity storage should support reserve()", "[entities]") {
    Storage storage;
    storage.reserve(555);
    REQUIRE(storage.capacity() >= 555);
}

TEST_CASE("Entity storage should support handing out pointers", "[entities]") {
    Storage storage;
    const Key k1 = storage.push_back(10);
    const Key k2 = storage.push_back(20);

    auto p1 = storage.ptr_to(k1);
    REQUIRE(*p1 == 10);

    auto p2 = storage.ptr_to(k2);
    REQUIRE(*p2 == 20);
}

TEST_CASE("Entity storage should replace all elements during reset", "[entities]") {
    Storage storage;

    const Key k1 = storage.push_back(1);
    REQUIRE(storage[k1] == 1);

    storage.reset(2, -1);
    REQUIRE(storage.size() == 2);
    REQUIRE(storage[k1] == -1);
}

} // namespace tiro::test
