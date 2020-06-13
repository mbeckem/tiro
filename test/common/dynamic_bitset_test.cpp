#include <catch.hpp>

#include "common/dynamic_bitset.hpp"

using namespace tiro;

using Set = DynamicBitset;

TEST_CASE("Dynamic bitset should support initial size", "[dynamic-bitset]") {
    Set s(16);
    REQUIRE(s.size() == 16);
    REQUIRE(s.count() == 0);
}

TEST_CASE("Dynamic bitset should support dynamic size", "[dynamic-bitset]") {
    Set s(16);

    s.resize(33, true);
    REQUIRE(s.size() == 33);
    REQUIRE(s.count() == 17);

    s.resize(15);
    REQUIRE(s.size() == 15);
    REQUIRE(s.count() == 0);

    s.grow(55, false);
    REQUIRE(s.size() == 55);
    REQUIRE(s.count() == 0);

    s.grow(54);
    REQUIRE(s.size() == 55);
}

TEST_CASE("Dynamic bitset should support setting and clearing of bits", "[dynamic-bitset]") {
    Set s(16);

    s.set(15);
    REQUIRE(s.test(15));
    REQUIRE(s.count() == 1);

    s.set(3);
    REQUIRE(s.test(3));
    REQUIRE(s.count() == 2);

    s.set(3, false);
    REQUIRE_FALSE(s.test(3));
    REQUIRE(s.count() == 1);

    s.clear(15);
    REQUIRE_FALSE(s.test(15));
    REQUIRE(s.count() == 0);
}

TEST_CASE("Dynamic bitset should support flipping single bits", "[dynamic-bitset]") {
    Set s(16);

    s.flip(15);
    REQUIRE(s.test(15));

    s.flip(15);
    REQUIRE_FALSE(s.test(15));
}

TEST_CASE("Dynamic bitset should support flipping all bits", "[dynamic-bitset]") {
    Set s(999);

    s.flip();
    REQUIRE(s.count() == 999);

    s.flip();
    REQUIRE(s.count() == 0);
}

TEST_CASE("Dynamic bitset should be able to find set bits", "[dynamic-bitset]") {
    Set s(999);
    s.set(3);
    s.set(7);
    s.set(11);
    s.set(23);
    s.set(123);
    s.set(998);

    std::vector<size_t> expected{3, 7, 11, 23, 123, 998};
    std::vector<size_t> found;
    {
        size_t start = 0;
        while (1) {
            size_t pos = s.find_set(start);
            if (pos == Set::npos)
                break;

            found.push_back(pos);
            start = pos + 1;
        }
    }

    REQUIRE(found == expected);
}

TEST_CASE("Dynamic bitset should be able to find unset bits", "[dynamic-bitset]") {
    Set s(999);
    s.set(3);
    s.set(7);
    s.set(11);
    s.set(23);
    s.set(123);
    s.set(998);
    s.flip();

    std::vector<size_t> expected{3, 7, 11, 23, 123, 998};
    std::vector<size_t> found;
    {
        size_t start = 0;
        while (1) {
            size_t pos = s.find_unset(start);
            if (pos == Set::npos)
                break;

            found.push_back(pos);
            start = pos + 1;
        }
    }

    REQUIRE(found == expected);
}
