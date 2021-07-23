#include <catch2/catch.hpp>

#include "eval_test.hpp"

namespace tiro::eval_tests {

TEST_CASE("Result should be able to represent successful values", "[results]") {
    std::string_view source = R"(
        import std;

        export func test_success() {
            const result = std.success(123);
            assert(result.type() == #success);
            assert(result.is_success());
            assert(!result.is_error());
            assert(result.value() == 123);
        }
    )";

    eval_test test(source);
    test.call("test_success").returns_null();
}

TEST_CASE("Result should be able to represent errors", "[results]") {
    std::string_view source = R"(
        import std;

        export func test_error() {
            const result = std.error("some error");
            assert(result.type() == #error);
            assert(!result.is_success());
            assert(result.is_error());
            assert(result.error() == "some error");
        }
    )";

    eval_test test(source);
    test.call("test_error").returns_null();
}

TEST_CASE("Accessing the wrong result member results in a runtime error", "[results]") {
    std::string_view source = R"(
        import std;

        export func test_success() {
            const result = std.success(123);
            return result.reason();
        }

        export func test_error() {
            const result = std.error("some error");
            return result.value();
        }
    )";

    eval_test test(source);
    test.call("test_success").panics();
    test.call("test_error").panics();
}

} // namespace tiro::eval_tests