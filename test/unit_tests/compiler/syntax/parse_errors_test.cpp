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

TEST_CASE("Parser should report error on unexpected block in an invalid position", "[syntax]") {
    std::string_view source = R"(
        {

        }
    )";
    REQUIRE_THROWS_MATCHES(parse_file_syntax(source), BadSyntax,
        test_support::exception_contains_string("unexpected block"));
}

TEST_CASE("Parser should report error on unexpected and unterminated block in an invalid position",
    "[syntax]") {
    std::string_view source = R"(
        {
    )";
    REQUIRE_THROWS_MATCHES(parse_file_syntax(source), BadSyntax,
        test_support::exception_contains_string("unexpected block"));
}

TEST_CASE("Parser should report error on uppercase code", "[syntax]") {
    // Totally invalid but, parser should not throw an internal error
    std::string_view source = R"(
        IMPORT STD;

        EXPORT FUNC MAIN() {
            CONST OBJECT = "WORLD";
            STD.PRINT("HELLO ${OBJECT}!");
        }
    )";
    REQUIRE_THROWS_AS(parse_file_syntax(source), BadSyntax);
}

} // namespace tiro::test
