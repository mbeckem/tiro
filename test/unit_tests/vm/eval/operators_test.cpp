#include "./test_context.hpp"

namespace tiro::vm::test {

TEST_CASE("Integers and floats should support equality tests", "[eval]") {
    std::string_view source = R"(
        export func test(a, b) {
            return a == b;
        }
    )";

    TestContext test(source);
    test.call("test", 0, -0.0).returns_bool(true);
    test.call("test", 0, -1.0).returns_bool(false);
    test.call("test", -0.0, 0).returns_bool(true);
    test.call("test", -1.0, 0).returns_bool(false);
    test.call("test", 4, 4.5).returns_bool(false);
    test.call("test", 4.5, 4).returns_bool(false);
    test.call("test", 4.0, 4).returns_bool(true);
    test.call("test", 4, 4.0).returns_bool(true);
}

TEST_CASE("The language should support basic arithmetic operations", "[eval]") {
    std::string_view source = R"(
        export func add(x, y) = {
            x + y;
        }

        export func sub(x, y) = {
            x - y;
        }

        export func mul(x, y) = {
            x * y;
        }

        export func div(x, y) = {
            x / y;
        }

        export func mod(x, y) = {
            x % y;
        }

        export func pow(x, y) = {
            x ** y;
        }

        // TODO bitwise operations
    )";

    TestContext test(source);

    test.call("add", 3, 4).returns_int(7);
    test.call("add", 3.5, -4).returns_float(-0.5);

    test.call("sub", 3, 4).returns_int(-1);
    test.call("sub", 3, -4.5).returns_float(7.5);

    test.call("mul", 3, 4).returns_int(12);
    test.call("mul", 3, 4.5).returns_float(13.5);

    test.call("div", 7, 3).returns_int(2);
    test.call("div", 10, 4.0).returns_float(2.5);

    test.call("mod", 7, 3).returns_int(1);
    test.call("mod", 10, 4.0).returns_float(2.0);

    test.call("pow", 3, 4).returns_int(81);
    test.call("pow", 4, 0.5).returns_float(2);
}

TEST_CASE("The language should support basic logical operators", "[eval]") {
    std::string_view source = R"(
        export func not(x) = {
            !x;
        }
    )";

    TestContext test(source);
    test.call("not", true).returns_bool(false);
    test.call("not", 0).returns_bool(false);
    test.call("not", 1).returns_bool(false);
    test.call("not", 1.5).returns_bool(false);
    test.call("not", "foo").returns_bool(false);
    test.call("not", "").returns_bool(false);
    test.call("not", false).returns_bool(true);
    test.call("not", nullptr).returns_bool(true);
}

} // namespace tiro::vm::test
