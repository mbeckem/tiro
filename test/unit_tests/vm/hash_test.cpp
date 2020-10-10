#include <catch2/catch.hpp>

#include "common/span.hpp"
#include "vm/hash.hpp"

#include <algorithm>

using namespace tiro;
using namespace tiro::vm;

static bool same_bits(f64 a, f64 b) {
    const auto ra = raw_span(a);
    const auto rb = raw_span(b);
    return std::equal(ra.begin(), ra.end(), rb.begin(), rb.end());
}

TEST_CASE("Float values -0.0 and 0.0 should hash to the same value", "[hash]") {
    const auto pos_0 = +0.0;
    const auto neg_0 = -0.0;

    REQUIRE(std::signbit(pos_0) != std::signbit(neg_0));
    REQUIRE(float_hash(pos_0) == float_hash(neg_0));
}

TEST_CASE("All nan values should hash to the same value", "[hash]") {
    const auto n1 = std::nan("123");
    const auto n2 = std::nan("456");
    REQUIRE_FALSE(same_bits(n1, n2));
    REQUIRE(float_hash(n1) == float_hash(n2));
}
