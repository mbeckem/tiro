#include <catch.hpp>

#include "tiro/vm/context.hpp"
#include "tiro/vm/math.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Valid size values should be extracted from a value", "[math]") {
    Context ctx;
    Root<Value> v(ctx);

    v.set(SmallInteger::make(0));
    REQUIRE(try_extract_size(v).value() == 0);

    v.set(Integer::make(ctx, 0));
    REQUIRE(try_extract_size(v).value() == 0);

    v.set(ctx.get_integer(0x1234567890));
    REQUIRE(try_extract_size(v).value() == 0x1234567890);
}

TEST_CASE("Extracted sizes from invalid values should fail", "[math]") {
    Context ctx;
    Root<Value> v(ctx);

    v.set(SmallInteger::make(-1));
    REQUIRE_FALSE(try_extract_size(v).has_value());

    v.set(Integer::make(ctx, -1));
    REQUIRE_FALSE(try_extract_size(v).has_value());

    // Cannot test values larger than size_t max with i64 ...
}

// TODO other functions in math.hpp
