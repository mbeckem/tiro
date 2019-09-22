#include <catch.hpp>

#include "hammer/core/unicode.hpp"

using namespace hammer;

TEST_CASE("Unicode character categories", "[unicode]") {
    REQUIRE(general_category('A') == GeneralCategory::Lu);
    REQUIRE(general_category('z') == GeneralCategory::Ll);
    REQUIRE(general_category('0') == GeneralCategory::Nd);
    REQUIRE(general_category(0) == GeneralCategory::Cc);
    REQUIRE(general_category(u32(-1)) == GeneralCategory::Invalid);
    REQUIRE(general_category(1'114'112) == GeneralCategory::Invalid);
}
