#include <catch2/catch.hpp>

#include "common/id_type.hpp"

using namespace tiro;

TIRO_DEFINE_ID(MyId, u32);

TEST_CASE("Basic id type operations", "[id-type]") {
    MyId invalid{};
    REQUIRE(!invalid);
    REQUIRE(!invalid.valid());
    REQUIRE(invalid.value() == u32(-1));

    REQUIRE(MyId::invalid.value() == u32(-1));

    MyId valid1 = MyId(1);
    REQUIRE(valid1);
    REQUIRE(valid1.value() == 1);

    MyId valid2 = MyId(2);
    REQUIRE(valid2);
    REQUIRE(valid2.value() == 2);

    REQUIRE(invalid != valid1);
    REQUIRE(invalid != valid2);
    REQUIRE(valid1 != valid2);

    REQUIRE(valid1 == valid1);
    REQUIRE(invalid == invalid);

    REQUIRE(valid1 < valid2);
    REQUIRE(valid2 > valid1);

    REQUIRE(valid1 <= valid1);
    REQUIRE(valid1 >= valid1);
}
