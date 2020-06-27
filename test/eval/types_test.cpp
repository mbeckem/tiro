#include <catch.hpp>

#include "../support/test_context.hpp"

using namespace tiro::vm;

TEST_CASE("The type_of function should return the correct type.") {
    std::string_view source = R"(
        import std;

        export func test() {
            const map = Map{};
            const add = func(name, obj) {
                map[name] = std.type_of(obj).name();
            };

            add("array", []);
            add("true", true);
            add("false", false);
            add("coroutine", std.launch(func() {}));
            add("dynamic object", std.new_object());
            add("float", 1.5);
            add("function", func() {});
            add("imported function", std.print);
            add("map", Map{});
            add("huge integer", 2 ** 62);
            add("module", std);
            add("null", null);
            add("small integer", 1);
            add("string", "");
            add("string builder", std.new_string_builder());
            add("symbol", #foo);
            add("tuple", (1, 2));
            add("type", std.type_of(std.type_of(null)));
            return map;
        }
    )";

    // TODO: Native objects and functions not tested.

    TestContext test(source);

    auto map_result = test.call("test").run();
    auto map = map_result.handle().strict_cast<HashTable>();

    auto require_entry = [&](std::string_view key, std::string_view expected_name) {
        CAPTURE(key, expected_name);

        Root key_obj(test.ctx(), String::make(test.ctx(), key));
        Root expected_obj(test.ctx(), String::make(test.ctx(), expected_name));

        Root<Value> actual_obj(test.ctx());
        if (auto found = map->get(key_obj)) {
            actual_obj.set(*found);
        } else {
            FAIL("Failed to find key.");
        }

        REQUIRE(equal(actual_obj, expected_obj.get()));
    };

    require_entry("array", "Array");
    require_entry("true", "Boolean");
    require_entry("false", "Boolean");
    require_entry("coroutine", "Coroutine");
    require_entry("dynamic object", "DynamicObject");
    require_entry("float", "Float");
    require_entry("function", "Function");
    require_entry("imported function", "Function");
    require_entry("map", "Map");
    require_entry("huge integer", "Integer");
    require_entry("module", "Module");
    require_entry("null", "Null");
    require_entry("small integer", "Integer");
    require_entry("string", "String");
    require_entry("string builder", "StringBuilder");
    require_entry("symbol", "Symbol");
    require_entry("tuple", "Tuple");
    require_entry("type", "Type");
}

// TODO: Expose builtin types as constant module members and test their values here.
