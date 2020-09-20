#include <catch2/catch.hpp>

#include "common/iter_tools.hpp"

using namespace tiro;

TEST_CASE("Counting range should visit all integers in range", "[iter-tools]") {
    CountingRange<int> range(1, 12);
    std::vector<int> observed(range.begin(), range.end());

    std::vector<int> expected;
    for (int i = 1; i < 12; ++i)
        expected.push_back(i);
    REQUIRE(observed == expected);
}

TEST_CASE("Counting range with min == max should be empty", "[iter-tools]") {
    CountingRange<int> range(5, 5);
    REQUIRE(range.min() == 5);
    REQUIRE(range.max() == 5);
    REQUIRE(range.begin() == range.end());

    for ([[maybe_unused]] int i : range) {
        FAIL("Range must be empty.");
    }
}

TEST_CASE("Transform view should map its elements", "[iter-tools]") {
    std::vector<int> values{1, 3, 5, 9};
    TransformView view(range_view(values), [](int i) { return i * i; });
    std::vector<int> observed(view.begin(), view.end());
    std::vector<int> expected{1, 9, 25, 81};
    REQUIRE(observed == expected);
}
