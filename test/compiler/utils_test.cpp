#include <catch.hpp>

#include "compiler/utils.hpp"

using namespace tiro;

TEST_CASE("escape_string should escape special characters", "[utils]") {
    struct test {
        std::string_view input;
        std::string_view expected;
    };

    test tests[] = {{"hello world!", "hello world!"}, {u8"ööö", "\\xc3\\xb6\\xc3\\xb6\\xc3\\xb6"},
        {"test\nnew\r\nline\r", "test\\nnew\\r\\nline\\r"}};

    for (const test& t : tests) {
        CAPTURE(t.input);
        CAPTURE(t.expected);

        std::string output = escape_string(t.input);
        REQUIRE(output == t.expected);
    }
}
