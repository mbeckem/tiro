#include <catch.hpp>

#include "hammer/core/math.hpp"

using namespace hammer;

TEST_CASE("max_pow2 should return the correct power of 2", "[math]") {
    REQUIRE(max_pow2<u8>() == u8(1) << 7);
    REQUIRE(max_pow2<u16>() == u16(1) << 15);
    REQUIRE(max_pow2<u32>() == u32(1) << 31);
    REQUIRE(max_pow2<u64>() == u64(1) << 63);
}

TEST_CASE("ceil_pow2 should round up to the correct power", "[math]") {
    REQUIRE(ceil_pow2<u32>(0) == 0);
    REQUIRE(ceil_pow2<u32>(1) == 1);
    REQUIRE(ceil_pow2<u32>(3) == 4);
    REQUIRE(ceil_pow2<u32>(9999) == 16384);
    REQUIRE(ceil_pow2<u32>(u32(1) << 31) == (u32(1) << 31));
}
