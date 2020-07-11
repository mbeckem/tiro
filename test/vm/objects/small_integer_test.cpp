#include <catch.hpp>

#include "vm/context.hpp"
#include "vm/objects/primitives.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Small integer bounds should be enforced", "[small-integer]") {
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

    REQUIRE(SmallInteger::make(min).value() == min);
    REQUIRE(SmallInteger::make(max).value() == max);

    REQUIRE_THROWS(SmallInteger::make(min - 1));
    REQUIRE_THROWS(SmallInteger::make(max + 1));
}

TEST_CASE("Small integers should be constructible", "[small-integer]") {
    Context ctx;
    Scope sc(ctx);

    SmallInteger si1 = SmallInteger::make(0);
    REQUIRE(si1.is_embedded_integer());
    REQUIRE(!si1.is_heap_ptr());
    REQUIRE(si1.value() == 0);
    REQUIRE(equal(si1, si1));
    REQUIRE(si1.same(SmallInteger::make(0)));

    SmallInteger si2 = SmallInteger::make(1);
    REQUIRE(!si2.is_heap_ptr());
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

    SmallInteger si5 = SmallInteger::make(-1);
    REQUIRE(si5.is_embedded_integer());
    REQUIRE(!si5.is_heap_ptr());
    REQUIRE(si5.value() == -1);

    Local heap_int = sc.local(Integer::make(ctx, -123123));
    REQUIRE(equal(si4, heap_int.get()));
    REQUIRE(hash(heap_int.get()) == hash(si4));
}
