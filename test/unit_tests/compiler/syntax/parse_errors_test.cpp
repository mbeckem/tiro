#include <catch2/catch.hpp>

#include "./syntax_assert.hpp"
#include "support/matchers.hpp"

namespace tiro::test {

TEST_CASE("Parser does not crash on invalid expressions inside string literals", "[syntax]") {
    std::string_view source = R"(
        import std;

        export func main() {
            const object = "World";
            std.print("Hello ${object)}");
        }
    )";
    REQUIRE_THROWS_MATCHES(parse_file_syntax(source), BadSyntax,
        test_support::exception_contains_string("expected '}'"));
}

TEST_CASE("Parser does not crash on unexpected closing brace", "[syntax]") {
    std::string_view source = R"(
        import std;

        export func main(foo) {
            foo(});
        }
    )";

    REQUIRE_THROWS_MATCHES(parse_file_syntax(source), BadSyntax,
        test_support::exception_contains_string("expected an expression"));
}

TEST_CASE("Parser should report error on unclosed nested function", "[syntax]") {
    // The parser got stuck inside the unclosed "(" before
    std::string_view source = R"(
        export func main() {
            func(
        }
    )";
    REQUIRE_THROWS_MATCHES(parse_file_syntax(source), BadSyntax,
        test_support::exception_contains_string("expected ')'"));
}

} // namespace tiro::test
