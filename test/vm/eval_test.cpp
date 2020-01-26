#include <catch.hpp>

#include "tiro/objects/primitives.hpp"
#include "tiro/objects/strings.hpp"
#include "tiro/vm/context.hpp"
#include "tiro/vm/math.hpp"

#include "./eval_context.hpp"

#include <iostream>

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Functions should support explicit returns", "[eval]") {
    std::string_view source = R"(
        func return_value() {
            return 123;
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "return_value");
    REQUIRE(extract_integer(result) == 123);
}

TEST_CASE("Functions should support implicit returns", "[eval]") {
    std::string_view source = R"(
        func return_value() {
            4.0;
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "return_value");
    REQUIRE(result->is<Float>());
    REQUIRE(result->as<Float>().value() == 4.0);
}

TEST_CASE("Functions should support mixed returns", "[eval]") {
    std::string_view source = R"(
        func return_value(x) {
            if (x) {
                456;
            } else {
                2 * return "Hello";
            }
        }

        func return_number() {
            return_value(true);
        }

        func return_string() {
            return return_value(false);
        }
    )";

    TestContext test;

    auto number = test.compile_and_run(source, "return_number");
    REQUIRE(extract_integer(number) == 456);

    auto string = test.compile_and_run(source, "return_string");
    REQUIRE(string->is<String>());
    REQUIRE(string->as<String>().view() == "Hello");
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

    TestContext test;
    auto number = test.compile_and_run(source, "toplevel");
    REQUIRE(extract_integer(number) == 9);
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

    TestContext test;
    auto number = test.compile_and_run(source, "outer");
    REQUIRE(extract_integer(number) == 3);
}

TEST_CASE("Interpreter should be able to run recursive fibonacci", "[eval]") {
    std::string_view source = R"(
        func fibonacci_slow(i) {
            if (i <= 1) {
                return i;
            }
            return fibonacci_slow(i - 1) + fibonacci_slow(i - 2);
        }

        func run_fib() {
            fibonacci_slow(20);
        }
    )";

    TestContext test;

    auto result = test.compile_and_run(source, "run_fib");
    REQUIRE(extract_integer(result) == 6765);
}

TEST_CASE("Interpreter should be able to run iterative fibonacci", "[eval]") {
    std::string_view source = R"(
        func fibonacci_fast(i) {
            if (i <= 1) {
                return i;
            }

            var a = 0;
            var b = 1;
            while (i >= 2) {
                var c = a + b;
                a = b;
                b = c;
                i = i - 1;
            }
            return b;
        }

        func run_fib() {
            fibonacci_fast(80);
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "run_fib");
    REQUIRE(extract_integer(result) == 23416728348467685);
}

TEST_CASE(
    "Interpreter should be able to run the iterative fibonacci (tuple "
    "assignment version)",
    "[eval]") {
    std::string_view source = R"(
        func fibonacci_fast(i) {
            if (i <= 1) {
                return i;
            }

            var a = 0;
            var b = 1;
            while (i >= 2) {
                (a, b) = (b, a + b);
                i = i - 1;
            }
            return b;
        }

        func run_fib() {
            fibonacci_fast(80);
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "run_fib");
    REQUIRE(extract_integer(result) == 23416728348467685);
}

TEST_CASE("Interpreter should be able to run memoized fibonacci", "[eval]") {
    std::string_view source = R"(
        func fibonacci_memo() {
            const m = Map{};

            var fib;
            fib = func(i) {
                if (m.contains(i)) {
                    return m[i];
                }

                const result = if (i <= 1) {
                    i;
                } else {
                    fib(i - 1) + fib(i - 2);
                };
                return m[i] = result;
            };
            return fib;
        }

        func run_fib() {
            const fib = fibonacci_memo();
            return fib(80);
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "run_fib");
    REQUIRE(extract_integer(result) == 23416728348467685);
}

TEST_CASE("Interpreter should compute factorial using a for loop", "[eval]") {
    std::string_view source = R"(
        func factorial() {
            const n = 10;

            var fac = 1;
            for (var i = 2; i <= n; i = i + 1) {
                fac = fac * i;
            }
            return fac;
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "factorial");
    REQUIRE(extract_integer(result) == 3'628'800);
}

TEST_CASE("Interpreter should throw an exception on assert failure", "[eval]") {
    std::string_view source = R"(
        func tick() {
            assert(false, "boom!");
        }
    )";

    TestContext test;
    try {
        test.compile_and_run(source, "tick");
        FAIL("Must throw an error.");
    } catch (const Error& e) {
        std::string msg = e.what();
        bool found = msg.find("boom!") != std::string::npos;
        REQUIRE(found);
    } catch (...) {
        FAIL("Unexpected exception type.");
    }
}

TEST_CASE(
    "Interpreter should allow assertions with interpolated string contents",
    "[eval]") {
    std::string_view source = R"(
        func tick() {
            const x = "tick tick...";
            assert(false, "${x} boom!");
        }
    )";

    TestContext test;
    try {
        test.compile_and_run(source, "tick");
        FAIL("Must throw an error.");
    } catch (const Error& e) {
        std::string msg = e.what();
        bool found = msg.find("tick tick... boom!") != std::string::npos;
        REQUIRE(found);
    } catch (...) {
        FAIL("Unexpected exception type.");
    }
}

TEST_CASE("StringBuilder should be supported", "[eval]") {
    std::string_view source = R"(
        import std;

        func make_greeter(greeting) {
            return func(name) {
                const builder = std.new_string_builder();
                builder.append(greeting, " ", name, "!");
                builder.to_str();
            };
        }

        func show_greeting() {
            const greeter = make_greeter("Hello");
            return greeter("Marko");
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "show_greeting");
    REQUIRE(result->is<String>());
    REQUIRE(result->as<String>().view() == "Hello Marko!");
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

        func lots_of_calls() {
            recursive_count(10000);
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "lots_of_calls");
    REQUIRE(extract_integer(result) == 10000);
}

TEST_CASE(
    "Dynamic object's members should be inspectable and modifiable", "[eval]") {
    std::string_view source = R"(
        import std;

        func test_object() {
            const obj = std.new_object();
            obj.foo = 3;
            obj.foo * -1;
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "test_object");
    REQUIRE(extract_integer(result) == -3);
}

TEST_CASE("Dynamic object's members should be null when unset", "[eval]") {
    std::string_view source = R"(
        import std;

        func test_object() {
            const obj = std.new_object();
            obj.non_existing_property;
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "test_object");
    REQUIRE(result->is_null());
}

TEST_CASE("Dynamic object's member functions should be invokable", "[eval]") {
    std::string_view source = R"(
        import std;

        func test_object() {
            const obj = std.new_object();
            obj.function = func(x) {
                x * 2;
            };
            obj.function(3);
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "test_object");
    REQUIRE(extract_integer(result) == 6);
}

TEST_CASE("Methods of the map class should be callable", "[eval]") {
    std::string_view source = R"(
        func map_usage() {
            const m = Map{
                "key": "value",
                "rm": null,
            };
            m[1] = 2;
            m["key"] = "key";
            m[null] = 3;

            m.remove("rm");
            m[1] = m.contains(1);
            m[null] = m.contains("other_key");
            m;
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "map_usage");
    REQUIRE(result->is<HashTable>());

    auto table = result.handle().cast<HashTable>();
    REQUIRE(table->size() == 3);

    Context& ctx = test.ctx();

    // "key"
    {
        Root key(ctx, String::make(ctx, "key"));
        REQUIRE(table->contains(key));

        Root value(ctx, Value::null());
        if (auto found = table->get(key))
            value.set(*found);

        REQUIRE(value->is<String>());
        REQUIRE(value->as<String>().view() == "key");
    }

    // null
    {
        Root value(ctx, Value::null());
        if (auto found = table->get(Value::null()); found)
            value.set(*found);

        REQUIRE(value->same(ctx.get_boolean(false)));
    }

    // 1
    {
        Root key(ctx, ctx.get_integer(1));
        Root value(ctx, Value::null());
        if (auto found = table->get(key); found)
            value.set(*found);

        REQUIRE(value->same(ctx.get_boolean(true)));
    }
}

TEST_CASE("Buffer data should be accessable", "[eval]") {
    std::string_view source = R"(
        import std;

        func buffer_size() {
            const b = std.new_buffer(1234);
            return b.size();
        }

        func buffer_get() {
            const b = std.new_buffer(4096);
            b[4095];
        }

        func buffer_set() {
            const b = std.new_buffer(4096);
            b[123] = 64;
            return b[123];
        }
    )";

    TestContext test;

    {
        auto result = test.compile_and_run(source, "buffer_size");
        REQUIRE(extract_integer(result) == 1234);
    }

    {
        auto result = test.compile_and_run(source, "buffer_get");
        REQUIRE(extract_integer(result) == 0);
    }

    {
        auto result = test.compile_and_run(source, "buffer_set");
        REQUIRE(extract_integer(result) == 64);
    }
}

TEST_CASE("sequences of string literals should be merged", "[eval]") {
    std::string_view source = R"(
        func strings() {
            return "hello " "world";
        }
    )";

    TestContext test;

    auto result = test.compile_and_run(source, "strings");
    REQUIRE(result->is<String>());
    REQUIRE(result->as<String>().view() == "hello world");
}

TEST_CASE("tuple members should be accessible", "[eval]") {
    std::string_view source = R"(
        func tuple_members() {
            var tup = (1, (2, 3));
            tup.1.0 = 4;
            return tup.1.0;
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "tuple_members");
    REQUIRE(extract_integer(result) == 4);
}

TEST_CASE("Expression blocks should be evaluated correctly", "[eval]") {
    std::string_view source = R"(
        func identity(x) {
            return x;
        }

        func test() {
            return {
                const x = identity({
                    var foo = 4;
                    foo;
                });

                if (x) {
                    { x; }; // Intentionally stupid
                } else {
                    return -1;
                }
            };
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "test");
    REQUIRE(extract_integer(result) == 4);
}

TEST_CASE("Results of assignments are propagated", "[eval]") {
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

        func test() {
            return outer(0);
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "test");
    REQUIRE(extract_integer(result) == 123);
}

TEST_CASE("Assignment should be supported for left hand side tuple literals",
    "[eval]") {
    std::string_view source = R"(
        func test() {
            var a = 1;
            var b = 2;
            var c = 3;
            (a, b, c) = (c, a - b, b);
            return (a, b, c);
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "test");
    REQUIRE(result->is<Tuple>());

    auto tuple = result.handle().cast<Tuple>();
    REQUIRE(tuple->size() == 3);
    REQUIRE(extract_integer(tuple->get(0)) == 3);  // a
    REQUIRE(extract_integer(tuple->get(1)) == -1); // b
    REQUIRE(extract_integer(tuple->get(2)) == 2);  // c
}

TEST_CASE("Tuple assignment should work for function return values", "[eval]") {
    std::string_view source = R"(
        func test() {
            var a;
            var b;
            (a, b) = returns_tuple();
            (a, b);
        }

        func returns_tuple() {
            return (123, 456);
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "test");
    REQUIRE(result->is<Tuple>());

    auto tuple = result.handle().cast<Tuple>();
    REQUIRE(tuple->size() == 2);
    REQUIRE(extract_integer(tuple->get(0)) == 123); // a
    REQUIRE(extract_integer(tuple->get(1)) == 456); // b
}

TEST_CASE(
    "Tuple unpacking declarations should be evaluated correctly", "[eval]") {
    std::string_view source = R"(
            func test() {
                var (a, b, c) = returns_tuple();
                return (c, b, a);
            }

            func returns_tuple() {
                return (1, 2, 3);
            }
        )";

    TestContext test;
    auto result = test.compile_and_run(source, "test");
    REQUIRE(result->is<Tuple>());

    auto tuple = result.handle().cast<Tuple>();
    REQUIRE(tuple->size() == 3);

    REQUIRE(extract_integer(tuple->get(0)) == 3); // c
    REQUIRE(extract_integer(tuple->get(1)) == 2); // b
    REQUIRE(extract_integer(tuple->get(2)) == 1); // a
}

TEST_CASE("Multiple variables should be initialized correctly", "[eval]") {
    std::string_view source = R"(
        func test() {
            var a = 3, b = -1;
            return (a, b);
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "test");
    REQUIRE(result->is<Tuple>());

    auto tuple = result.handle().cast<Tuple>();
    REQUIRE(tuple->size() == 2);

    REQUIRE(extract_integer(tuple->get(0)) == 3);  // a
    REQUIRE(extract_integer(tuple->get(1)) == -1); // b
}

TEST_CASE("Interpolated strings should be evaluated correctly", "[eval]") {
    std::string_view source = R"RAW(
        func test() {
            const world = "World";
            return "Hello $world!";
        }
    )RAW";

    TestContext test;
    auto result = test.compile_and_run(source, "test");
    REQUIRE(result->is<String>());

    auto string = result.handle().cast<String>();
    REQUIRE(string->view() == "Hello World!");
}

TEST_CASE("The value of a tuple assignment should be the right hand side tuple",
    "[eval]") {
    std::string_view source = R"RAW(
        func test() {
            var a, b;
            return (a, b) = (1, 2, 3);
        }
    )RAW";

    TestContext test;
    auto result = test.compile_and_run(source, "test");
    REQUIRE(result->is<Tuple>());

    auto tuple = result.handle().cast<Tuple>();
    REQUIRE(tuple->size() == 3);
    REQUIRE(extract_integer(tuple->get(0)) == 1);
    REQUIRE(extract_integer(tuple->get(1)) == 2);
    REQUIRE(extract_integer(tuple->get(2)) == 3);
}

TEST_CASE("Array size should be returned correctly", "[eval]") {
    std::string_view source = R"RAW(
        func test_initial() {
            var array = [1, 2, 3, 4, 5];
            return array.size();
        }

        func test_empty() {
            return [].size();
        }

        func test_append() {
            var array = [1, 2];
            array.append("foo");
            return array.size();
        }
    )RAW";

    TestContext test;

    {
        auto result = test.compile_and_run(source, "test_initial");
        REQUIRE(extract_integer(result) == 5);
    }

    {
        auto result = test.compile_and_run(source, "test_empty");
        REQUIRE(extract_integer(result) == 0);
    }

    {
        auto result = test.compile_and_run(source, "test_append");
        REQUIRE(extract_integer(result) == 3);
    }
}

TEST_CASE("Tuple size should be returned correctly", "[eval]") {
    std::string_view source = R"RAW(
        func test_size() {
            const tuple = (1, 2, 3);
            return tuple.size();
        }

        func test_empty() {
            return ().size();
        }
    )RAW";

    TestContext test;

    {
        auto result = test.compile_and_run(source, "test_size");
        REQUIRE(extract_integer(result) == 3);
    }

    {
        auto result = test.compile_and_run(source, "test_empty");
        REQUIRE(extract_integer(result) == 0);
    }
}

TEST_CASE("Multiple variables in for loop initializer should be supported",
    "[eval]") {
    std::string_view source = R"RAW(
        func test() {
            const nums = [1, 2, 3, 4, 5];
            var sum = 0;

            for (var i = 0, n = nums.size(); i < n; i = i + 1) {
                sum = sum + nums[i];
            }
            sum;
        }        
    )RAW";

    TestContext test;
    auto result = test.compile_and_run(source, "test");
    REQUIRE(extract_integer(result) == 15);
}

TEST_CASE("Assignment operators should be evaluated correctly", "[eval]") {
    std::string_view source = R"RAW(
        func add() {
            var a = 4;
            a += 3;
        }

        func sub() {
            var a = 3;
            1 + (a -= 2);
            return a;
        }

        func mul() {
            var a = 9;
            return a *= 2;
        }

        func div() {
            var a = 4;
            return a /= (1 + 1);
        }

        func mod() {
            var a = 7;
            a %= 3;
        }

        func pow() {
            var a = 9;
            a **= 2;
            return a;
        }
    )RAW";

    TestContext test;

    auto verify_integer = [&](std::string_view function, i64 expected) {
        auto result = test.compile_and_run(source, function);
        CAPTURE(function, expected);
        REQUIRE(extract_integer(result) == expected);
    };

    verify_integer("add", 7);
    verify_integer("sub", 1);
    verify_integer("mul", 18);
    verify_integer("div", 2);
    verify_integer("mod", 1);
    verify_integer("pow", 81);
}

TEST_CASE(
    "Evaluation order should be strictly left to right", "[eval][!mayfail]") {
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

        func test_attribute() {
            const order = order_tester();

            const v1 = order.add("1", std.new_object());
            const v2 = order.add("2", "value");
            
            v1().key = v2();
            
            return order.get();
        }

        func test_subscript_get() {
            const order = order_tester();

            const array = [1, 2];

            const v1 = order.add("1", array);
            const v2 = order.add("2", 1);
            
            v1()[v2()];

            return order.get();
        }

        func test_subscript_set() {
            const order = order_tester();

            const array = [1, 2, 3];

            const v1 = order.add("1", array);
            const v2 = order.add("2", 1);
            const v3 = order.add("3", 2);
            
            v1()[v2()] = v3();

            return order.get();
        }

        func test_call() {
            const order = order_tester();
            
            const v1 = order.add("1", func(x, y) {});
            const v2 = order.add("2", 0);
            const v3 = order.add("3", 1);

            v1()(v2(), v3());

            return order.get();
        }

        func test_method() {
            const order = order_tester();

            const object = std.new_object();
            object.method = func(x, y) {};

            const v1 = order.add("1", object);
            const v2 = order.add("2", 1);
            const v3 = order.add("3", 2);

            v1().method(v2(), v3());

            return order.get();
        }

        func test_tuple_assign() {
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

        func test_nested() {
            const order = order_tester();

            const v1 = order.add("1", 1);
            const v2 = order.add("2", 2);
            const v3 = order.add("3", func(x, y) { x + y; });
            const v4 = order.add("4", 4);
            const v5 = order.add("5", 5);
            const v6 = order.add("6", 6);

            -v1() + v2() * v3()(v4(), v5()) ** v6();

            return order.get();
        }
    )RAW";

    TestContext test;

    {
        auto result = test.compile_and_run(source, "test_attribute");
        REQUIRE(result->is<String>());
        REQUIRE(result.handle().cast<String>()->view() == "12");
    }

    {
        auto result = test.compile_and_run(source, "test_subscript_get");
        REQUIRE(result->is<String>());
        REQUIRE(result.handle().cast<String>()->view() == "12");
    }

    {
        auto result = test.compile_and_run(source, "test_subscript_set");
        REQUIRE(result->is<String>());
        REQUIRE(result.handle().cast<String>()->view() == "123");
    }

    {
        auto result = test.compile_and_run(source, "test_call");
        REQUIRE(result->is<String>());
        REQUIRE(result.handle().cast<String>()->view() == "123");
    }

    {
        auto result = test.compile_and_run(source, "test_method");
        REQUIRE(result->is<String>());
        REQUIRE(result.handle().cast<String>()->view() == "123");
    }

    {
        auto result = test.compile_and_run(source, "test_tuple_assign");
        REQUIRE(result->is<String>());
        REQUIRE(result.handle().cast<String>()->view() == "1234");
    }

    {
        auto result = test.compile_and_run(source, "test_nested");
        REQUIRE(result->is<String>());
        REQUIRE(result.handle().cast<String>()->view() == "123456");
    }
}
