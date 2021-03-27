#include <catch2/catch.hpp>

#include "common/entities/entity_id.hpp"

namespace tiro::test {

namespace {

TIRO_DEFINE_ENTITY_ID(MyId, u32);

} // namespace

static_assert(sizeof(MyId) == sizeof(u32));

TEST_CASE("Basic entity id operations", "[entities]") {
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

} // namespace tiro::test
