#include <catch2/catch.hpp>

#include "common/adt/vec_ptr.hpp"

using namespace tiro;

TEST_CASE("VecPtr basic operations", "[vec_ptr]") {
    std::vector<int> vec{1, 2, 3};

    VecPtr v0(vec, 0);
    REQUIRE(v0.valid());
    REQUIRE(v0.get() == &vec[0]);
    REQUIRE(*v0 == 1);
    REQUIRE(v0 == v0);

    VecPtr v2(vec, 2);
    REQUIRE(v2.valid());
    REQUIRE(v2.get() == &vec[2]);
    REQUIRE(*v2 == 3);

    REQUIRE(!(v0 == v2));
    REQUIRE(v0 != v2);
    REQUIRE(v0 < v2);
    REQUIRE(!(v0 > v2));
    REQUIRE(v0 <= v2);
    REQUIRE(!(v0 >= v2));
}

TEST_CASE("Invalid VecPtr behaviour", "[vec_ptr]") {
    VecPtr<int, std::vector<int>> invalid1 = nullptr;
    VecPtr<int, std::vector<int>> invalid2;

    REQUIRE(!invalid1.valid());
    REQUIRE(!invalid2.valid());

    REQUIRE(invalid1.get() == nullptr);
    REQUIRE(invalid2.get() == nullptr);

    REQUIRE(invalid1 == invalid2);
    REQUIRE(!(invalid1 != invalid2));
    REQUIRE(!(invalid1 < invalid2));
    REQUIRE(!(invalid1 > invalid2));
    REQUIRE(invalid1 >= invalid2);
    REQUIRE(invalid1 <= invalid2);
}
