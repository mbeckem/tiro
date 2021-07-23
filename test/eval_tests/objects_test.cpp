#include <catch2/catch.hpp>

#include "eval_test.hpp"

namespace tiro::eval_tests {

TEST_CASE("Records should be constructible through syntax", "[objects]") {
    std::string_view source = R"(
        export func test() {
            return (foo: "x", bar: 3);
        }
    )";

    eval_test test(source);

    auto result = test.call("test").returns_value().as<record>();
    auto keys = result.keys();
    REQUIRE(keys.size() == 2);
    REQUIRE(keys.get(0).as<string>().view() == "bar");
    REQUIRE(keys.get(1).as<string>().view() == "foo");

    auto foo = result.get(make_string(test.get_vm(), "foo"));
    REQUIRE(foo.as<string>().view() == "x");

    auto bar = result.get(make_string(test.get_vm(), "bar"));
    REQUIRE(bar.as<integer>().value() == 3);
}

TEST_CASE("Record's members should be inspectable and modifiable", "[objects]") {
    std::string_view source = R"(
        import std;

        export func test_record() {
            const rec = (foo: 2);
            rec.foo = 3;
            return rec.foo * -1;
        }
    )";

    eval_test test(source);
    test.call("test_record").returns_int(-3);
}

TEST_CASE("Record's members should be null by default", "[objects]") {
    std::string_view source = R"(
        import std;

        export func test_record() = {
            const rec = std.new_record([#foo]);
            rec.foo;
        }
    )";

    eval_test test(source);
    test.call("test_record").returns_null();
}

TEST_CASE("Record's member functions should be invokable", "[objects]") {
    std::string_view source = R"(
        import std;

        export func test_record() = {
            const rec = (
                function: func(x) = x * 2
            );
            rec.function(3);
        }
    )";

    eval_test test(source);
    test.call("test_record").returns_int(6);
}

} // namespace tiro::eval_tests