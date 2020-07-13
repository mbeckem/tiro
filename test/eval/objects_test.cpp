#include "support/test_context.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Dynamic object's members should be inspectable and modifiable", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test_object() {
            const obj = std.new_object();
            obj.foo = 3;
            return obj.foo * -1;
        }
    )";

    TestContext test(source);
    test.call("test_object").returns_int(-3);
}

TEST_CASE("Dynamic object's members should be null when unset", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test_object() = {
            const obj = std.new_object();
            obj.non_existing_property;
        }
    )";

    TestContext test(source);
    test.call("test_object").returns_null();
}

TEST_CASE("Dynamic object's member functions should be invokable", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test_object() = {
            const obj = std.new_object();
            obj.function = func(x) = {
                x * 2;
            };
            obj.function(3);
        }
    )";

    TestContext test(source);
    test.call("test_object").returns_int(6);
}

TEST_CASE("Result should be able to represent successful values", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test_success() {
            const result = std.new_success(123);
            assert(result.type() == #success);
            assert(result.is_success());
            assert(!result.is_error());
            assert(result.value() == 123);
        }
    )";

    TestContext test(source);
    test.call("test_success").returns_null();
}

TEST_CASE("Result should be able to represent errors", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test_error() {
            const result = std.new_error("some error");
            assert(result.type() == #error);
            assert(!result.is_success());
            assert(result.is_error());
            assert(result.error() == "some error");
        }
    )";

    TestContext test(source);
    test.call("test_error").returns_null();
}

TEST_CASE("Accessing the wrong result member results in a runtime error", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test_success() {
            const result = std.new_success(123);
            return result.error();
        }

        export func test_error() {
            const result = std.new_error("some error");
            return result.value();
        }
    )";

    TestContext test(source);
    test.call("test_success").throws();
    test.call("test_error").throws();
}
