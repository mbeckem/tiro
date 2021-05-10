#include "./test_context.hpp"

#include "support/vm_matchers.hpp"

namespace tiro::vm::test {

using test_support::is_integer_value;

TEST_CASE("Simple variables should be supported", "[eval]") {
    std::string_view source = R"(
        export func test(n) {
            const x = n;
            var z = x - 1;
            z = z * 2;
            return z;
        }
    )";

    TestContext test(source);
    test.call("test", 5).returns_int(8);
}

TEST_CASE("Multiple variables should be initialized correctly", "[eval]") {
    std::string_view source = R"(
        export func test() {
            var a = 3, b = -1;
            return (a, b);
        }
    )";

    TestContext test(source);
    auto result = test.call("test").returns_value();
    REQUIRE(result->is<Tuple>());

    auto tuple = result.must_cast<Tuple>();
    REQUIRE(tuple->size() == 2);

    REQUIRE_THAT(tuple->get(0), is_integer_value(3));  // a
    REQUIRE_THAT(tuple->get(1), is_integer_value(-1)); // b
}

TEST_CASE("Results of assignments should be propagated", "[eval]") {
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

    TestContext test(source);
    test.call("test").returns_int(123);
}

TEST_CASE("The value of a tuple assignment should be the right hand side tuple", "[eval]") {
    std::string_view source = R"RAW(
        export func test() {
            var a, b;
            return (a, b) = (1, 2, 3);
        }
    )RAW";

    TestContext test(source);
    auto result = test.call("test").returns_value();
    REQUIRE(result->is<Tuple>());

    auto tuple = result.must_cast<Tuple>();
    REQUIRE(tuple->size() == 3);
    REQUIRE_THAT(tuple->get(0), is_integer_value(1));
    REQUIRE_THAT(tuple->get(1), is_integer_value(2));
    REQUIRE_THAT(tuple->get(2), is_integer_value(3));
}

TEST_CASE("Assignment should be supported for left hand side tuple literals", "[eval]") {
    std::string_view source = R"(
        export func test() {
            var a = 1;
            var b = 2;
            var c = 3;
            (a, b, c) = (c, a - b, b);
            return (a, b, c);
        }
    )";

    TestContext test(source);
    auto result = test.call("test").returns_value();
    REQUIRE(result->is<Tuple>());

    auto tuple = result.must_cast<Tuple>();
    REQUIRE(tuple->size() == 3);
    REQUIRE_THAT(tuple->get(0), is_integer_value(3));  // a
    REQUIRE_THAT(tuple->get(1), is_integer_value(-1)); // b
    REQUIRE_THAT(tuple->get(2), is_integer_value(2));  // c
}

TEST_CASE("Tuple assignment should work for function return values", "[eval]") {
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

    TestContext test(source);
    auto result = test.call("test").returns_value();
    REQUIRE(result->is<Tuple>());

    auto tuple = result.must_cast<Tuple>();
    REQUIRE(tuple->size() == 2);
    REQUIRE_THAT(tuple->get(0), is_integer_value(123)); // a
    REQUIRE_THAT(tuple->get(1), is_integer_value(456)); // b
}

TEST_CASE("Tuple unpacking declarations should be evaluated correctly", "[eval]") {
    std::string_view source = R"(
            export func test() {
                var (a, b, c) = returns_tuple();
                return (c, b, a);
            }

            func returns_tuple() {
                return (1, 2, 3);
            }
        )";

    TestContext test(source);
    auto result = test.call("test").returns_value();
    REQUIRE(result->is<Tuple>());

    auto tuple = result.must_cast<Tuple>();
    REQUIRE(tuple->size() == 3);

    REQUIRE_THAT(tuple->get(0), is_integer_value(3)); // c
    REQUIRE_THAT(tuple->get(1), is_integer_value(2)); // b
    REQUIRE_THAT(tuple->get(2), is_integer_value(1)); // a
}

TEST_CASE("Assignment operators should be evaluated correctly", "[eval]") {
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

    TestContext test(source);

    auto verify_integer = [&](std::string_view function, i64 argument, i64 expected) {
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

} // namespace tiro::vm::test
