#include <catch.hpp>

#include "hammer/vm/context.hpp"
#include "hammer/vm/math.hpp"
#include "hammer/vm/objects/primitives.hpp"
#include "hammer/vm/objects/strings.hpp"

#include "./eval_context.hpp"

#include <iostream>

using namespace hammer;
using namespace hammer::vm;

TEST_CASE("return value from function (explicit)", "[eval]") {
    std::string source = R"(
        func return_value() {
            return 123;
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "return_value");
    REQUIRE(extract_integer(result) == 123);
}

TEST_CASE("return value from function (implicit)", "[eval]") {
    std::string source = R"(
        func return_value() {
            4.0;
        }
    )";

    TestContext test;
    auto result = test.compile_and_run(source, "return_value");
    REQUIRE(result->is<Float>());
    REQUIRE(result->as<Float>().value() == 4.0);
}

TEST_CASE("return value from function (mixed)", "[eval]") {
    std::string source = R"(
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

TEST_CASE("nested functions with closure", "[eval]") {
    std::string source = R"(
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

TEST_CASE("run fibonacci function (recursive)", "[eval]") {
    std::string source = R"(
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

TEST_CASE("run fibonacci function (iterative)", "[eval]") {
    std::string source = R"(
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

TEST_CASE("run fibonacci function (memoization)", "[eval]") {
    std::string source = R"(
        func fibonacci_memo() {
            const m = Map{};

            // TODO: Variables itself visible in initializers?
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

TEST_CASE("import module and build string", "[eval]") {
    std::string source = R"(
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
TEST_CASE("large number of recursive calls", "[eval]") {
    std::string source = R"(
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

TEST_CASE("dynamic object's members can be set and retrieved", "[eval]") {
    std::string source = R"(
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

TEST_CASE("dynamic object's members are null when unset", "[eval]") {
    std::string source = R"(
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

TEST_CASE("dynamic object's members can be invoked", "[eval]") {
    std::string source = R"(
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

TEST_CASE("methods of the map class can be called", "[eval]") {
    std::string source = R"(
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
