#include "support/test_context.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Functions should support explicit returns", "[eval]") {
    std::string_view source = R"(
        export func return_value() = {
            return 123;
        }
    )";

    TestContext test(source);
    test.call("return_value").returns_int(123);
}

TEST_CASE("Functions should support implicit returns", "[eval]") {
    std::string_view source = R"(
        export func return_value() = {
            4.0;
        }
    )";

    TestContext test(source);
    test.call("return_value").returns_float(4.0);
}

TEST_CASE("Functions with implicit return can use arbitary expressions", "[eval]") {
    std::string_view source = R"(
        func twice(a) = 2 * a;

        export func return_value(a, b) = twice(a) + {
            var c = b + 1;
            c;   
        };
    )";

    TestContext test(source);
    test.call("return_value", 2, 3).returns_int(8);
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

        export func return_number() {
            return return_value(true);
        }

        export func return_string() {
            return return_value(false);
        }
    )";

    TestContext test(source);
    test.call("return_number").returns_int(456);
    test.call("return_string").returns_string("Hello");
}

TEST_CASE("Interpreter should support nested functions and closures", "[eval]") {
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

        export func toplevel() {
            return helper(3);
        }
    )";

    TestContext test(source);
    test.call("toplevel").returns_int(9);
}

TEST_CASE("Interpreter should support closure variables in loops", "[eval]") {
    std::string_view source = R"(
        import std;

        export func outer() {
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
TEST_CASE("Interpreter should support a large number of recursive calls", "[eval]") {
    std::string_view source = R"(
        func recursive_count(n) {
            if (n <= 0) {
                return n;
            }

            return 1 + recursive_count(n - 1);
        }

        export func lots_of_calls() = {
            recursive_count(10000);
        }
    )";

    TestContext test(source);
    test.call("lots_of_calls").returns_int(10000);
}

TEST_CASE("The interpreter should bind method references to their instance", "[eval]") {
    std::string_view source = R"(
        import std;

        export func construct_bound() {
            const builder = std.new_string_builder();
            const bound = std.new_record([#append, #to_string]);
            bound.append = builder.append;
            bound.to_string = builder.to_string;
            return bound;
        }

        export func test_bound_method_syntax(bound) {
            bound.append();
            bound.append("foo");
            bound.append("_", "bar");
            return bound.to_string();
        }

        export func test_bound_function_syntax(bound) {
            const append = bound.append;
            const to_string = bound.to_string;
            append();
            append("!", "!");
            return to_string();
        }
    )";

    TestContext test(source);
    auto bound = test.call("construct_bound").run();
    test.call("test_bound_method_syntax", bound).returns_string("foo_bar");
    test.call("test_bound_function_syntax", bound).returns_string("foo_bar!!");
}
