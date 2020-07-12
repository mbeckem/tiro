#include "support/test_context.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("StringBuilder should be supported", "[eval]") {
    std::string_view source = R"(
        import std;

        func make_greeter(greeting) {
            return func(name) = {
                const builder = std.new_string_builder();
                builder.append(greeting, " ", name, "!");
                builder.to_string();
            };
        }

        export func show_greeting() {
            const greeter = make_greeter("Hello");
            return greeter("Marko");
        }
    )";

    TestContext test(source);
    test.call("show_greeting").returns_string("Hello Marko!");
}

TEST_CASE("Sequences of string literals should be merged", "[eval]") {
    std::string_view source = R"(
        export func strings() {
            return "hello " "world";
        }
    )";

    TestContext test(source);
    test.call("strings").returns_string("hello world");
}

TEST_CASE("Interpolated strings should be evaluated correctly", "[eval]") {
    std::string_view source = R"RAW(
        export func test(who) {
            return "Hello $who!";
        }
    )RAW";

    TestContext test(source);
    test.call("test", "World").returns_string("Hello World!");
}

TEST_CASE("Strings should be sliceable", "[eval]") {
    std::string_view source = R"RAW(
        export func slice_first(str) {
            return str.slice_first(5).to_string();
        }

        export func slice_last(str) {
            return str.slice_last(5).to_string();
        }

        export func slice(str) {
            return str.slice(3, 2).to_string();
        }
    )RAW";

    TestContext test(source);
    test.call("slice_first", "Hello World").returns_string("Hello");
    test.call("slice_last", "Hello World").returns_string("World");
    test.call("slice", "Hello World").returns_string("lo");
}
