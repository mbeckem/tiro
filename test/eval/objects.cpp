#include "./eval_context.hpp"

using namespace tiro;
using namespace tiro::vm;

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

    TestContext test(source);
    test.call("test_object").returns_int(-3);
}

TEST_CASE("Dynamic object's members should be null when unset", "[eval]") {
    std::string_view source = R"(
        import std;

        func test_object() {
            const obj = std.new_object();
            obj.non_existing_property;
        }
    )";

    TestContext test(source);
    test.call("test_object").returns_null();
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

    TestContext test(source);
    test.call("test_object").returns_int(6);
}
