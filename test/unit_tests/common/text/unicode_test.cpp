#include <catch2/catch.hpp>

#include "common/text/unicode.hpp"

namespace tiro::test {

TEST_CASE("Unicode letters should be recognized", "[unicode]") {
    REQUIRE(is_xid_start('A'));
    REQUIRE(is_xid_start('b'));
    REQUIRE(is_xid_start('z'));
    REQUIRE(is_xid_start('Z'));
    REQUIRE(is_xid_start(0x00F6));
    REQUIRE(is_xid_start(0x4E16)); // 世

    REQUIRE_FALSE(is_xid_start(0));
    REQUIRE_FALSE(is_xid_start('3'));
    REQUIRE_FALSE(is_xid_start('!'));
    REQUIRE_FALSE(is_xid_start(' '));
}

TEST_CASE("Unicode whitespace code points should be recognized", "[unicode]") {
    REQUIRE(is_whitespace(' '));
    REQUIRE(is_whitespace('\r'));
    REQUIRE(is_whitespace('\n'));

    REQUIRE_FALSE(is_whitespace('0'));
    REQUIRE_FALSE(is_whitespace('!'));
    REQUIRE_FALSE(is_whitespace('a'));
    REQUIRE_FALSE(is_whitespace(0x00F6)); // ö
    REQUIRE_FALSE(is_whitespace(0x4E16)); // 世
}

} // namespace tiro::test
