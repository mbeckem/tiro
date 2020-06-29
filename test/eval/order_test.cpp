#include "support/test_context.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Operators &&, || and ?? should short-circuit", "[eval]") {
    std::string_view source = R"RAW(
        import std;

        func order_tester() {
            const obj = std.new_object();
            const builder = std.new_string_builder();

            obj.add = func(str, value) {
                return func() {
                    builder.append(str);
                    return value;
                };
            };
            obj.get = func() {
                return builder.to_str();
            };

            return obj;
        }

        func result(str, r) {
            const v = if (r == true) {
                "t";
            } else if (r == false) {
                "f";
            } else if (r == null) {
                "n";
            } else {
                "<unexpected>";
            };
            return "$str$v";
        }

        export func test_and(a, b, c) {
            const order = order_tester();

            const v1 = order.add("a", a);
            const v2 = order.add("b", b);
            const v3 = order.add("c", c);
            const r = v1() && v2() && v3();

            return result(order.get(), r);
        }

        export func test_or(a, b, c) {
            const order = order_tester();

            const v1 = order.add("a", a);
            const v2 = order.add("b", b);
            const v3 = order.add("c", c);
            const r = v1() || v2() || v3();

            return result(order.get(), r);
        }

        export func test_coalesce(a, b, c) {
            const order = order_tester();

            const v1 = order.add("a", a);
            const v2 = order.add("b", b);
            const v3 = order.add("c", c);
            const r = v1() ?? v2() ?? v3();

            return result(order.get(), r);
        }
    )RAW";

    TestContext test(source);
    auto handle_true = test.make_boolean(true);
    auto handle_false = test.make_boolean(false);

    const auto require = [&](std::string_view function, auto a, auto b, auto c,
                             std::string_view expected) {
        CAPTURE(function, a, b, c);
        CAPTURE(expected);
        test.call(function, a, b, c).returns_string(expected);
    };

    require("test_and", true, true, true, "abct");
    require("test_and", true, true, false, "abcf");
    require("test_and", true, false, true, "abf");
    require("test_and", true, false, false, "abf");
    require("test_and", false, true, true, "af");
    require("test_and", false, true, false, "af");
    require("test_and", false, false, true, "af");
    require("test_and", false, false, false, "af");

    require("test_or", true, true, true, "at");
    require("test_or", true, true, false, "at");
    require("test_or", true, false, true, "at");
    require("test_or", true, false, false, "at");
    require("test_or", false, true, true, "abt");
    require("test_or", false, true, false, "abt");
    require("test_or", false, false, true, "abct");
    require("test_or", false, false, false, "abcf");

    require("test_coalesce", nullptr, nullptr, nullptr, "abcn");
    require("test_coalesce", nullptr, nullptr, true, "abct");
    require("test_coalesce", nullptr, true, false, "abt");
    require("test_coalesce", false, true, true, "af");
}

TEST_CASE("Evaluation order should be strictly left to right", "[eval]") {
    std::string_view source = R"RAW(
        import std;

        func order_tester() {
            const obj = std.new_object();
            const builder = std.new_string_builder();

            obj.add = func(str, value) {
                return func() {
                    builder.append(str);
                    return value;
                };
            };
            obj.get = func() {
                return builder.to_str();
            };

            return obj;
        }

        export func test_attribute() {
            const order = order_tester();

            const v1 = order.add("1", std.new_object());
            const v2 = order.add("2", "value");

            v1().key = v2();

            return order.get();
        }

        export func test_subscript_get() {
            const order = order_tester();

            const array = [1, 2];

            const v1 = order.add("1", array);
            const v2 = order.add("2", 1);

            v1()[v2()];

            return order.get();
        }

        export func test_subscript_set() {
            const order = order_tester();

            const array = [1, 2, 3];

            const v1 = order.add("1", array);
            const v2 = order.add("2", 1);
            const v3 = order.add("3", 2);

            v1()[v2()] = v3();

            return order.get();
        }

        export func test_call() {
            const order = order_tester();

            const v1 = order.add("1", func(x, y) {});
            const v2 = order.add("2", 0);
            const v3 = order.add("3", 1);

            v1()(v2(), v3());

            return order.get();
        }

        export func test_method() {
            const order = order_tester();

            const object = std.new_object();
            object.method = func(x, y) {};

            const v1 = order.add("1", object);
            const v2 = order.add("2", 1);
            const v3 = order.add("3", 2);

            v1().method(v2(), v3());

            return order.get();
        }

        export func test_tuple_assign() {
            const order = order_tester();

            const object = std.new_object();
            object.a = 1;

            var x = 3;

            const array = [1, 2, 3, 4];

            const v1 = order.add("1", object);
            const v2 = order.add("2", (0, 1));
            const v3 = order.add("3", [1, 2, 3, 4]);
            const v4 = order.add("4", 3);

            (v1().a, x, v2().1, v3()[v4()]) = (1, 2, 3, 4);

            return order.get();
        }

        export func test_tuple_literal() {
            const order = order_tester();

            const v1 = order.add("1", 1);
            const v2 = order.add("2", 2);
            const v3 = order.add("3", 3);

            const tuple = (v1(), v2(), v3());

            return order.get();
        }

        export func test_array_literal() {
            const order = order_tester();

            const v1 = order.add("1", 1);
            const v2 = order.add("2", 2);
            const v3 = order.add("3", 3);

            const array = [v1(), v2(), v3()];

            return order.get();
        }

        export func test_map_literal() {
            const order = order_tester();

            const v1 = order.add("1", 1);
            const v2 = order.add("2", 2);
            const v3 = order.add("3", 3);
            const v4 = order.add("4", 4);

            const map = map{
                v1(): v2(),
                v3(): v4(),
            };

            return order.get();
        }

        export func test_nested() {
            const order = order_tester();

            const v1 = order.add("1", 1);
            const v2 = order.add("2", 2);
            const v3 = order.add("3", func(x, y) = { x + y; });
            const v4 = order.add("4", 4);
            const v5 = order.add("5", 5);
            const v6 = order.add("6", 6);

            -v1() + v2() * v3()(v4(), v5()) ** v6();

            return order.get();
        }
    )RAW";

    // TODO: Set literal (not implemented yet).

    TestContext test(source);
    test.call("test_attribute").returns_string("12");
    test.call("test_subscript_get").returns_string("12");
    test.call("test_subscript_set").returns_string("123");
    test.call("test_call").returns_string("123");
    test.call("test_method").returns_string("123");
    test.call("test_tuple_assign").returns_string("1234");
    test.call("test_tuple_literal").returns_string("123");
    test.call("test_array_literal").returns_string("123");
    test.call("test_map_literal").returns_string("1234");
    test.call("test_nested").returns_string("123456");
}
