#include <catch2/catch.hpp>

#include "eval_test.hpp"
#include "matchers.hpp"

namespace tiro::eval_tests {

TEST_CASE("Constants at module scope should be supported", "[modules]") {
    std::string_view source = R"(
        const x = 3;
        const y = "world";
        const z = "Hello $y!";

        export func get_x() { return x; }
        export func get_y() { return y; }
        export func get_z() { return z; }
    )";

    eval_test test(source);
    test.call("get_x").returns_int(3);
    test.call("get_y").returns_string("world");
    test.call("get_z").returns_string("Hello world!");
}

TEST_CASE("Variables on module scope should be supported", "[modules]") {
    std::string_view source = R"(
        var foo = 1;

        export func test() {
            return foo += 1;
        }
    )";

    eval_test test(source);
    test.call("test").returns_int(2);
    test.call("test").returns_int(3);
    test.call("test").returns_int(4);
}

TEST_CASE("Complex init logic at module scope should be possible", "[modules]") {
    std::string_view source = R"(
        const data = [1, 2, 3, "end"];

        export const next = {
            var index = 0;

            func next() {
                var result = data[index];
                index += 1;
                return result;
            };
        };
    )";

    eval_test test(source);
    test.call("next").returns_int(1);
    test.call("next").returns_int(2);
    test.call("next").returns_int(3);
    test.call("next").returns_string("end");
}

TEST_CASE("Functions and variables in the same module can see each other", "[modules]") {
    std::string file_1 = R"(
        const data = [1, 2, 3];
    )";

    std::string file_2 = R"(
        export func get_1() = data[1];
    )";

    std::string file_3 = R"(
        export func get_2() {
            return get_1() + data[2];
        }
    )";

    eval_test test({file_1, file_2, file_3});
    test.call("get_1").returns_int(2);
    test.call("get_2").returns_int(5);
}

TEST_CASE("Importing the same module from multiple files does not produce an error", "[modules]") {
    std::string file_1 = R"(
        import std;

        export func a() {
            return std.PI;
        }
    )";

    std::string file_2 = R"(
        import std;

        export func b() {
            return std.print;
        }
    )";

    eval_test test({file_1, file_2});
    auto pi = test.call("a").returns_value();
    REQUIRE(pi.kind() == value_kind::float_);

    auto print = test.call("b").returns_value();
    REQUIRE(print.kind() == value_kind::function);
}

TEST_CASE("Imports cannot be seen from another file in the same module", "[modules]") {
    std::string file_1 = R"(
        import std;

        export func a() {
            return std.PI;
        }
    )";

    std::string file_2 = R"(
        export func b() {
            return std.print;
        }
    )";

    REQUIRE_THROWS_MATCHES(
        eval_test({file_1, file_2}), compile_error, throws_compile_error(api_errc::bad_source));
}

TEST_CASE("Redeclaring a symbol at module scope produces an error", "[modules]") {
    std::string file_1 = R"(
        const data = [1, 2, 3];
    )";

    std::string file_2 = R"(
        export func data() = 1;
    )";

    REQUIRE_THROWS_MATCHES(
        eval_test({file_1, file_2}), compile_error, throws_compile_error(api_errc::bad_source));
}

} // namespace tiro::eval_tests
