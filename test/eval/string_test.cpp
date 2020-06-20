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
                builder.to_str();
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
