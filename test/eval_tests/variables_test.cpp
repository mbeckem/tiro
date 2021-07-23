#include <catch2/catch.hpp>

#include "eval_test.hpp"

namespace tiro::eval_tests {

TEST_CASE("Simple variables should be supported", "[variables]") {
    std::string_view source = R"(
        export func test(n) {
            const x = n;
            var z = x - 1;
            z = z * 2;
            return z;
        }
    )";

    eval_test test(source);
    test.call("test", 5).returns_int(8);
}

TEST_CASE("Multiple variables should be initialized correctly", "[variables]") {
    std::string_view source = R"(
        export func test() {
            var a = 3, b = -1;
            return (a, b);
        }
    )";

    eval_test test(source);
    auto result = test.call("test").returns_value().as<tuple>();
    REQUIRE(result.size() == 2);
    REQUIRE(result.get(0).as<integer>().value() == 3);  // a
    REQUIRE(result.get(1).as<integer>().value() == -1); // b
}

TEST_CASE("Results of assignments should be propagated", "[variables]") {
    std::string_view source = R"(
        func outer(x) {
            const inner = func() {
                var a;
                var b = [0];
                var c = (0,);
                return x = a = b[0] = c.0 = 123;
            };
            return inner();
        }

        export func test() {
            return outer(0);
        }
    )";

    eval_test test(source);
    test.call("test").returns_int(123);
}

TEST_CASE("The value of a tuple assignment should be the right hand side tuple", "[variables]") {
    std::string_view source = R"RAW(
        export func test() {
            var a, b;
            return (a, b) = (1, 2, 3);
        }
    )RAW";

    eval_test test(source);
    auto result = test.call("test").returns_value().as<tuple>();

    REQUIRE(result.size() == 3);
    REQUIRE(result.get(0).as<integer>().value() == 1);
    REQUIRE(result.get(1).as<integer>().value() == 2);
    REQUIRE(result.get(2).as<integer>().value() == 3);
}

TEST_CASE("Assignment should be supported for left hand side tuple literals", "[variables]") {
    std::string_view source = R"(
        export func test() {
            var a = 1;
            var b = 2;
            var c = 3;
            (a, b, c) = (c, a - b, b);
            return (a, b, c);
        }
    )";

    eval_test test(source);
    auto result = test.call("test").returns_value().as<tuple>();
    REQUIRE(result.size() == 3);
    REQUIRE(result.get(0).as<integer>().value() == 3);  // a
    REQUIRE(result.get(1).as<integer>().value() == -1); // b
    REQUIRE(result.get(2).as<integer>().value() == 2);  // c
}

TEST_CASE("Tuple assignment should work for function return values", "[variables]") {
    std::string_view source = R"(
        export func test() = {
            var a;
            var b;
            (a, b) = returns_tuple();
            (a, b);
        }

        func returns_tuple() {
            return (123, 456);
        }
    )";

    eval_test test(source);
    auto result = test.call("test").returns_value().as<tuple>();

    REQUIRE(result.size() == 2);
    REQUIRE(result.get(0).as<integer>().value() == 123); // a
    REQUIRE(result.get(1).as<integer>().value() == 456); // b
}

TEST_CASE("Tuple unpacking declarations should be evaluated correctly", "[variables]") {
    std::string_view source = R"(
            export func test() {
                var (a, b, c) = returns_tuple();
                return (c, b, a);
            }

            func returns_tuple() {
                return (1, 2, 3);
            }
        )";

    eval_test test(source);
    auto result = test.call("test").returns_value().as<tuple>();
    REQUIRE(result.size() == 3);

    REQUIRE(result.get(0).as<integer>().value() == 3); // c
    REQUIRE(result.get(1).as<integer>().value() == 2); // b
    REQUIRE(result.get(2).as<integer>().value() == 1); // a
}

TEST_CASE("Assignment operators should be evaluated correctly", "[variables]") {
    std::string_view source = R"RAW(
        export func add(x) = {
            var a = x;
            a += 3;
        }

        export func sub(x) = {
            var a = x;
            1 + (a -= 2);
            return a;
        }

        export func mul(x) = {
            var a = x;
            return a *= 2;
        }

        export func div(x) = {
            var a = x;
            return a /= (1 + 1);
        }

        export func mod(x) = {
            var a = x;
            a %= 3;
        }

        export func pow(x) = {
            var a = x;
            a **= 2;
            return a;
        }
    )RAW";

    eval_test test(source);

    auto verify_integer = [&](const char* function, int64_t argument, int64_t expected) {
        CAPTURE(function, argument, expected);
        test.call(function, argument).returns_int(expected);
    };

    verify_integer("add", 4, 7);
    verify_integer("sub", 3, 1);
    verify_integer("mul", 9, 18);
    verify_integer("div", 4, 2);
    verify_integer("mod", 7, 1);
    verify_integer("pow", 9, 81);
}

} // namespace tiro::eval_tests