#include <catch.hpp>

#include "tiro/core/math.hpp"

using namespace tiro;

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

TEST_CASE(
    "checked_cast<> should return the value for valid conversions", "[math]") {
    // Unsigned -> Unsigned
    REQUIRE(checked_cast<byte>(u64(128)) == 128);

    // Unsigned -> Signed
    REQUIRE(checked_cast<i32>(u64(12345)) == 12345);

    // Signed -> Unsigned
    REQUIRE(checked_cast<byte>(i32(42)) == 42);

    // Signed -> Signed
    REQUIRE(checked_cast<i8>(-1) == -1);
}

TEST_CASE("checked_cast<> should throw for invalid conversions", "[math]") {
    // Unsigned -> Unsigned
    REQUIRE_THROWS(checked_cast<byte>(u64(-1)));
    REQUIRE_THROWS(checked_cast<byte>(u64(256)));

    // Unsigned -> Signed
    REQUIRE_THROWS(checked_cast<i32>(u64(-1)));
    REQUIRE_THROWS(checked_cast<i32>(u64(1) << 32));

    // Signed -> Unsigned
    REQUIRE_THROWS(checked_cast<byte>(i32(-1)));
    REQUIRE_THROWS(checked_cast<byte>(256));

    // Signed -> Signed
    REQUIRE_THROWS(checked_cast<i8>(-129));
    REQUIRE_THROWS(checked_cast<i8>(128));
}

TEST_CASE("checked_div shoud protect against errors", "[math]") {
    struct fail {};

    auto div = [](auto a, auto b) {
        if (!checked_div(a, b))
            throw fail();
        return a;
    };

    REQUIRE(div(int(11), 2) == 5);
    REQUIRE_THROWS_AS(div(int(123), int(0)), fail);
    REQUIRE_THROWS_AS(div(std::numeric_limits<int>::min(), -1), fail);

    REQUIRE(div(u64(99), u64(10)) == 9);
    REQUIRE_THROWS_AS(div(u64(123456), u64(0)), fail);
}

TEST_CASE("checked_mod shoud protect against errors", "[math]") {
    struct fail {};

    auto mod = [](auto a, auto b) {
        if (!checked_mod(a, b))
            throw fail();
        return a;
    };

    REQUIRE(mod(int(11), 2) == 1);
    REQUIRE_THROWS_AS(mod(int(123), int(0)), fail);
    REQUIRE_THROWS_AS(mod(std::numeric_limits<int>::min(), -1), fail);

    REQUIRE(mod(u64(99), u64(10)) == 9);
    REQUIRE_THROWS_AS(mod(u64(123456), u64(0)), fail);
}
