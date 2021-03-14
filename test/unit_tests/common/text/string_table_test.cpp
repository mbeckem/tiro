#include <catch2/catch.hpp>

#include "common/text/string_table.hpp"

namespace tiro::test {

TEST_CASE("StringTable should be able to create and deduplicate strings", "[string-table]") {
    StringTable strings;

    auto s1 = strings.insert("Hello");
    REQUIRE(s1);
    REQUIRE(s1.value() == 1);
    REQUIRE(strings.value(s1) == "Hello");

    auto s2 = strings.insert("Hello");
    REQUIRE(s2);
    REQUIRE(s2.value() == s1.value()); // Same string
    REQUIRE(strings.value(s2) == "Hello");

    auto s3 = strings.insert("World");
    REQUIRE(s3);
    REQUIRE(s3.value() != s1.value());
    REQUIRE(strings.value(s3) == "World");

    REQUIRE(strings.size() == 2);

    auto s4 = strings.find("Hello");
    REQUIRE(s4);
    REQUIRE(s4.value() == s1);

    auto s5 = strings.find("World");
    REQUIRE(s5);
    REQUIRE(s5.value() == s3);

    auto s6 = strings.find("Does not exist");
    REQUIRE_FALSE(s6);
}

} // namespace tiro::test
