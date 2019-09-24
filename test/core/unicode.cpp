#include <catch.hpp>

#include "hammer/core/unicode.hpp"

using namespace hammer;

TEST_CASE("Unicode character categories", "[unicode]") {
    REQUIRE(general_category('A') == GeneralCategory::Lu);
    REQUIRE(general_category('z') == GeneralCategory::Ll);
    REQUIRE(general_category('0') == GeneralCategory::Nd);
    REQUIRE(general_category(0) == GeneralCategory::Cc);
    REQUIRE(general_category(u32(-1)) == GeneralCategory::Invalid);
    REQUIRE(general_category(1'114'111) == GeneralCategory::Cn);
    REQUIRE(general_category(1'114'112) == GeneralCategory::Invalid);
}

TEST_CASE("Unicode letter", "[unicode]") {
    REQUIRE(is_letter('A'));
    REQUIRE(is_letter('b'));
    REQUIRE(is_letter('z'));
    REQUIRE(is_letter('Z'));
    REQUIRE(is_letter(0x00F6)); // ö
    REQUIRE(is_letter(0x4E16)); // 世

    REQUIRE_FALSE(is_letter(0));
    REQUIRE_FALSE(is_letter('3'));
    REQUIRE_FALSE(is_letter('!'));
    REQUIRE_FALSE(is_letter(' '));
}

TEST_CASE("Unicode whitespace", "[unicode]") {
    REQUIRE(is_whitespace(' '));
    REQUIRE(is_whitespace('\r'));
    REQUIRE(is_whitespace('\n'));
    // TODO special whitespace; negative case
}
