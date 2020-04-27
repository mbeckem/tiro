#include "./eval_context.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Functions should support explicit returns", "[eval]") {
    std::string_view source = R"(
        func return_value() = {
            return 123;
        }
    )";

    TestContext test(source);
    test.call("return_value").returns_int(123);
}

TEST_CASE("Functions should support implicit returns", "[eval]") {
    std::string_view source = R"(
        func return_value() = {
            4.0;
        }
    )";

    TestContext test(source);
    test.call("return_value").returns_float(4.0);
}

TEST_CASE("Functions should support mixed returns", "[eval]") {
    std::string_view source = R"(
        func return_value(x) = {
            if (x) {
                456;
            } else {
                2 * return "Hello";
            }
        }

        func return_number() {
            return return_value(true);
        }

        func return_string() {
            return return_value(false);
        }
    )";

    TestContext test(source);
    test.call("return_number").returns_int(456);
    test.call("return_string").returns_string("Hello");
}

TEST_CASE(
    "Interpreter should support nested functions and closures", "[eval]") {
    std::string_view source = R"(
        func helper(a) {
            var b = 0;
            var c = 1;
            const nested = func() {
                return a + b;
            };

            while (1) {
                var d = 3;

                const nested2 = func() {
                    return nested() + d + a;
                };

                return nested2();
            }
        }

        func toplevel() {
            return helper(3);
        }
    )";

    TestContext test(source);
    test.call("toplevel").returns_int(9);
}

TEST_CASE("Interpreter should support closure variables in loops", "[eval]") {
    std::string_view source = R"(
        import std;

        func outer() {
            var b = 2;
            while (1) {
                var a = 1;
                var f = func() {
                    return a + b;
                };
                return f();
            }
        }
    )";

    TestContext test(source);
    test.call("outer").returns_int(3);
}

// TODO implement and test tail recursion
TEST_CASE(
    "Interpreter should support a large number of recursive calls", "[eval]") {
    std::string_view source = R"(
        func recursive_count(n) {
            if (n <= 0) {
                return n;
            }

            return 1 + recursive_count(n - 1);
        }

        func lots_of_calls() = {
            recursive_count(10000);
        }
    )";

    TestContext test(source);
    test.call("lots_of_calls").returns_int(10000);
}
