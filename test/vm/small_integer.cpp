#include <catch.hpp>

#include "hammer/vm/context.hpp"
#include "hammer/vm/objects/small_integer.hpp"

using namespace hammer;
using namespace hammer::vm;

TEST_CASE("Small integer bounds", "[small-integer]") {
    i64 min = 0;
    i64 max = 0;

    if constexpr (sizeof(uintptr_t) == 8) {
        min = -(i64(1) << 62);
        max = (i64(1) << 62) - 1;
    } else if constexpr (sizeof(uintptr_t) == 4) {
        min = -(i64(1) << 30);
        max = (i64(1) << 30) - 1;
    } else {
        FAIL("Unsupported architecture pointer size.");
    }
    REQUIRE(SmallInteger::min == min);
    REQUIRE(SmallInteger::max == max);
}

TEST_CASE("Small integer construction", "[small-integer]") {
    Context ctx;

    SmallInteger si1 = SmallInteger::make(0);
    REQUIRE(si1.is_embedded_integer());
    REQUIRE(si1.value() == 0);
    REQUIRE(equal(si1, si1));
    REQUIRE(si1.same(SmallInteger::make(0)));

    SmallInteger si2 = SmallInteger::make(1);
    REQUIRE(si2.is_embedded_integer());
    REQUIRE(si2.value() == 1);

    SmallInteger si3 = SmallInteger::make(1);
    REQUIRE(si3.is_embedded_integer());
    REQUIRE(si3.value() == 1);

    REQUIRE(equal(si2, si3));
    REQUIRE(hash(si2) == hash(si3));

    SmallInteger si4 = SmallInteger::make(-123123);
    REQUIRE(si4.is_embedded_integer());
    REQUIRE(si4.value() == -123123);
    REQUIRE(!equal(si4, si3));

    Root<Integer> heap_int(ctx, Integer::make(ctx, -123123));
    REQUIRE(equal(si4, heap_int.get()));
    REQUIRE(hash(heap_int.get()) == hash(si4));
}
