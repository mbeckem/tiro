#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/math.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Valid size values should be extracted from a value", "[math]") {
    Context ctx;
    Scope sc(ctx);
    Local v = sc.local();

    v = SmallInteger::make(0);
    REQUIRE(try_extract_size(*v).value() == 0);

    v = Integer::make(ctx, 0);
    REQUIRE(try_extract_size(*v).value() == 0);

    v = ctx.get_integer(0x1234567890);
    REQUIRE(try_extract_size(*v).value() == 0x1234567890);
}

TEST_CASE("Extracted sizes from invalid values should fail", "[math]") {
    Context ctx;
    Scope sc(ctx);
    Local v = sc.local();

    v = SmallInteger::make(-1);
    REQUIRE_FALSE(try_extract_size(*v).has_value());

    v = Integer::make(ctx, -1);
    REQUIRE_FALSE(try_extract_size(*v).has_value());

    // Cannot test values larger than size_t max with i64 ...
}

TEST_CASE("Integer pow should return the expected results", "[math]") {
    struct Test {
        i64 lhs;
        i64 rhs;
        i64 expected;
    };

    Test tests[] = {
        {0, 0, 1},
        {1, 0, 1},
        {5, 0, 1},
        {-99, 0, 1},

        {1, -1, 1},
        {1, -123, 1},
        {2, -1, 0},
        {2, -123, 0},

        {-1, 1, -1},
        {-1, -1, -1},
        {-2, -1, 0},

        {3, 4, 81},
        {11, 14, 379749833583241},
        {-11, 14, 379749833583241},
        {-11, 13, -34522712143931},
    };

    Context ctx;
    Scope sc(ctx);
    Local a = sc.local();
    Local b = sc.local();
    Local c = sc.local();
    for (const auto& test : tests) {
        CAPTURE(test.lhs, test.rhs, test.expected);

        a = ctx.get_integer(test.lhs);
        b = ctx.get_integer(test.rhs);
        c = pow(ctx, a, b);

        i64 result = extract_integer(*c);
        REQUIRE(result == test.expected);
    }
}

TEST_CASE("Integer pow should throws on invalid input", "[math]") {
    struct Test {
        i64 lhs;
        i64 rhs;
    };

    Test tests[] = {
        {0, -1},
        {123, 777},
        {2, 64},
        {-2, 64},
    };

    Context ctx;
    Scope sc(ctx);
    Local a = sc.local();
    Local b = sc.local();
    for (const auto& test : tests) {
        CAPTURE(test.lhs, test.rhs);

        a = ctx.get_integer(test.lhs);
        b = ctx.get_integer(test.rhs);
        // TODO handle overflow errors differently (lang exceptions?)
        REQUIRE_THROWS_AS(pow(ctx, a, b), Error);
    }
}

// TODO other functions in math.hpp
