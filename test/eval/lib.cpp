#include "support/test_context.hpp"

using namespace tiro::vm;

TEST_CASE("Result should be able to represent successful values", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test_success() {
            const result = std.success(123);
            assert(result.type() == #success);
            assert(result.is_success());
            assert(!result.is_failure());
            assert(result.value() == 123);
        }
    )";

    TestContext test(source);
    test.call("test_success").returns_null();
}

TEST_CASE("Result should be able to represent errors", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test_error() {
            const result = std.failure("some error");
            assert(result.type() == #failure);
            assert(!result.is_success());
            assert(result.is_failure());
            assert(result.reason() == "some error");
        }
    )";

    TestContext test(source);
    test.call("test_error").returns_null();
}

TEST_CASE("Accessing the wrong result member results in a runtime error", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test_success() {
            const result = std.success(123);
            return result.reason();
        }

        export func test_error() {
            const result = std.failure("some error");
            return result.value();
        }
    )";

    TestContext test(source);
    test.call("test_success").throws();
    test.call("test_error").throws();
}

TEST_CASE("The current coroutine should be accessible", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test() {
            return std.current_coroutine().name();
        }
    )";

    TestContext test(source);
    test.call("test").returns_string("Coroutine-1");
}

TEST_CASE("The type_of function should return the correct type.", "[eval]") {
    std::string_view source = R"(
        import std;

        // Constructs map of `name -> (actual_type, expected_type)`.
        export func test() {
            const map = map{};
            const add = func(name, obj, expected) {
                map[name] = (std.type_of(obj), expected);
            };

            add("array", [], std.Array);
            add("true", true, std.Boolean);
            add("false", false, std.Boolean);
            add("coroutine", std.launch(func() {}), std.Coroutine);
            add("dynamic object", std.new_object(), std.DynamicObject);
            add("float", 1.5, std.Float);
            add("function", func() {}, std.Function);
            add("imported function", std.print, std.Function);
            add("bound function", "123".size, std.Function);
            add("map", map{}, std.Map);
            add("map key view", map{}.keys(), std.MapKeyView);
            add("map value view", map{}.values(), std.MapValueView);
            add("huge integer", 2 ** 62, std.Integer);
            add("module", std, std.Module);
            add("null", null, std.Null);
            add("result", std.success(123), std.Result);
            add("small integer", 1, std.Integer);
            add("string", "", std.String);
            add("string builder", std.new_string_builder(), std.StringBuilder);
            add("string slice", "hello world".slice_first(5), std.StringSlice);
            add("symbol", #foo, std.Symbol);
            add("tuple", (1, 2), std.Tuple);
            add("type", std.type_of(std.type_of(null)), std.Type);
            return map;
        }
    )";

    // TODO: Native objects and functions not tested.

    TestContext test(source);
    Context& ctx = test.ctx();

    auto map_result = test.call("test").run();
    auto map = map_result.handle().must_cast<HashTable>();

    auto require_entry = [&](std::string_view key, std::string_view expected_name) {
        CAPTURE(key, expected_name);

        Scope sc(ctx);
        Local key_obj = sc.local(String::make(test.ctx(), key));
        Local actual_obj = sc.local(map->get(*key_obj).value_or(Value::null()));

        if (!actual_obj->is<Tuple>())
            FAIL("Expected a tuple.");

        Handle<Tuple> tuple = actual_obj.must_cast<Tuple>();
        REQUIRE(tuple->size() == 2);

        Local actual = sc.local(tuple->get(0));
        Local expected = sc.local(tuple->get(1));
        REQUIRE(actual->is<Type>());
        REQUIRE(actual->must_cast<Type>().name().view() == expected_name);
        REQUIRE(actual->same(*expected));
    };

    require_entry("array", "Array");
    require_entry("true", "Boolean");
    require_entry("false", "Boolean");
    require_entry("coroutine", "Coroutine");
    require_entry("dynamic object", "DynamicObject");
    require_entry("float", "Float");
    require_entry("function", "Function");
    require_entry("imported function", "Function");
    require_entry("bound function", "Function");
    require_entry("map", "Map");
    require_entry("map key view", "MapKeyView");
    require_entry("map value view", "MapValueView");
    require_entry("huge integer", "Integer");
    require_entry("module", "Module");
    require_entry("null", "Null");
    require_entry("result", "Result");
    require_entry("small integer", "Integer");
    require_entry("string", "String");
    require_entry("string builder", "StringBuilder");
    require_entry("string slice", "StringSlice");
    require_entry("symbol", "Symbol");
    require_entry("tuple", "Tuple");
    require_entry("type", "Type");
}
