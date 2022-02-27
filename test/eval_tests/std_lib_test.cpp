#include <catch2/catch.hpp>

#include "eval_test.hpp"

#include <string>
#include <unordered_map>

namespace tiro::eval_tests {

TEST_CASE("The debug representation of builtin objects should be as expected", "[std-lib]") {
    std::string_view source = R"END(
        import std;

        export func test() {
            const r = std.debug_repr;

            // Primitives
            assert(r(null) == "null");
            assert(r(true) == "true");
            assert(r(false) == "false");
            assert(r(1) == "1");
            assert(r(1.0) == "1.0");
            assert(r(-13.37) == "-13.37");
            assert(r("hello") == "\"hello\"");
            assert(r("hello\n\r\t'\"\\") == "\"hello\\n\\r\\t\\'\\\"\\\\\"");
            assert(r("\x00") == "\"\\x00\""); // NUL
            assert(r("\u{E007F}") == "\"\\u{E007F}\""); // Cancel Tag U+E007F
            assert(r(#foo) == "#foo");

            // TODO: Test control characters (ASCII and unicode) in strings. We don't have a way to input them with literal syntax yet.

            // Builtin structs
            assert(r(std.Integer) == "Type{name: \"Integer\"}");
            assert(r(std.success(1)) == "Result{type: \"success\", value: 1, error: null}");
            assert(r("hello world".slice_first(5)) == "StringSlice{value: \"hello\"}");

            // Containers
            assert(r(()) == "()");
            assert(r((1,)) == "(1,)");
            assert(r((1,2,3)) == "(1, 2, 3)");
            assert(r((:)) == "(:)");
            assert(r((foo: 1, bar: 2)) == "(bar: 2, foo: 1)"); // VM happens to sort keys in static record schemas at the moment
            assert(r([]) == "[]");
            assert(r([1,2]) == "[1, 2]");
            assert(r(map{}) == "map{}");
            assert(r(map{1:2,3:4}) == "map{1: 2, 3: 4}");
            assert(r(set{}) == "set{}");
            assert(r(set{1, 1, 2}) == "set{1, 2}");
        }
    )END";

    eval_test test(source);
    test.call("test").returns_null();
}

TEST_CASE("Debug representation should support pretty printing", "[std-lib]") {
    std::string_view source = R"END(
        import std;

        export func test() {
            const r = func(v) = std.debug_repr(v, true);

            // Builtin structs
            assert(r(std.Integer) == "Type{\n    name: \"Integer\"\n}");
            assert(r(std.success(std.Integer)) == "Result{\n    type: \"success\",\n    value: Type{\n        name: \"Integer\"\n    },\n    error: null\n}");

            // Containers
            assert(r(()) == "()");
            assert(r((1,)) == "(\n    1,\n)");
            assert(r((1,2,3)) == "(\n    1,\n    2,\n    3\n)");
            assert(r((:)) == "(:)");
            assert(r((foo: 1, bar: 2)) == "(\n    bar: 2,\n    foo: 1\n)"); // VM happens to sort keys in static record schemas at the moment
            assert(r([]) == "[]");
            assert(r([1,2]) == "[\n    1,\n    2\n]");
            assert(r(map{}) == "map{}");
            assert(r(map{1:2,3:4}) == "map{\n    1: 2,\n    3: 4\n}");
            assert(r(set{}) == "set{}");
            assert(r(set{1, 1, 2}) == "set{\n    1,\n    2\n}");
        }
    )END";

    eval_test test(source);
    test.call("test").returns_null();
}

TEST_CASE("The debug representation should handle cylcic data structures", "[std-lib]") {
    std::string_view source = R"END(
        import std;

        export func test() {
            const m = map{};
            m[1] = m;

            assert(std.debug_repr(m) == "map{1: {...}}");
        }
    )END";

    eval_test test(source);
    test.call("test").returns_null();
}

TEST_CASE("The type_of function should return the correct type.", "[std-lib]") {
    std::string_view source = R"(
        import std;

        // Constructs an array of `(name, actual_type, expected_type)`.
        export func test() {
            const result = [];
            const add = func(name, obj, expected) {
                result.append((name, std.type_of(obj), expected));
            };

            add("array", [], std.Array);
            add("true", true, std.Boolean);
            add("false", false, std.Boolean);
            add("coroutine", std.launch(func() {}), std.Coroutine);
            add("coroutine token", std.coroutine_token(), std.CoroutineToken);
            add("exception", get_exception(), std.Exception);
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
            add("record", (foo: "bar"), std.Record);
            add("record schema", std.schema_of((foo: "bar")), std.RecordSchema);
            add("result", std.success(123), std.Result);
            add("set", set{1, 2, 3}, std.Set);
            add("small integer", 1, std.Integer);
            add("string", "", std.String);
            add("string builder", std.new_string_builder(), std.StringBuilder);
            add("string slice", "hello world".slice_first(5), std.StringSlice);
            add("symbol", #foo, std.Symbol);
            add("tuple", (1, 2), std.Tuple);
            add("type", std.type_of(std.type_of(null)), std.Type);
            return result;
        }

        func get_exception() {
            const r = std.catch_panic(func() = std.panic("help!"));
            return r.error();
        }
    )";

    // TODO: Native objects and functions not tested.

    eval_test test(source);

    struct entry {
        type actual_type;
        type expected_type;
    };

    auto result = test.call("test").returns_value().as<array>();
    std::unordered_map<std::string, entry> entries;
    for (size_t i = 0, n = result.size(); i < n; ++i) {
        CAPTURE(i);
        auto value = result.get(i).as<tuple>();
        auto id = value.get(0).as<string>();
        auto actual_type = value.get(1).as<type>();
        auto expected_type = value.get(2).as<type>();
        entries.emplace(id.value(), entry{actual_type, expected_type});
    }

    auto require_entry = [&](std::string id, std::string_view expected_name) {
        CAPTURE(id, expected_name);

        auto& entry = entries.at(id);
        REQUIRE(entry.actual_type.name().as<string>().view() == expected_name);
        REQUIRE(same(test.get_vm(), entry.actual_type, entry.expected_type));
    };

    require_entry("array", "Array");
    require_entry("true", "Boolean");
    require_entry("false", "Boolean");
    require_entry("coroutine", "Coroutine");
    require_entry("coroutine token", "CoroutineToken");
    require_entry("exception", "Exception");
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
    require_entry("record", "Record");
    require_entry("record schema", "RecordSchema");
    require_entry("result", "Result");
    require_entry("set", "Set");
    require_entry("small integer", "Integer");
    require_entry("string", "String");
    require_entry("string builder", "StringBuilder");
    require_entry("string slice", "StringSlice");
    require_entry("symbol", "Symbol");
    require_entry("tuple", "Tuple");
    require_entry("type", "Type");
}

TEST_CASE("The return values of builtin math functions should be correct", "[std-lib]") {
    std::string_view source = R"(
        import std;

        export func test() {
            assert(approx_eq(5, 5.0001));
            assert(!approx_eq(5, 6));
            assert(approx_eq(-5, -5.0001));
            assert(!approx_eq(-5, -6));

            assert(approx_eq(std.PI, 3.14159));
            assert(approx_eq(std.TAU, 6.28318));
            assert(approx_eq(std.E, 2.71828));
            assert(2.0 ** 64 < std.INFINITY);

            assert(std.abs(1) == 1);
            assert(std.abs(-1) == 1);

            assert(std.pow(2, 3) == 8);

            assert(approx_eq(std.log(std.E), 1));
            assert(approx_eq(std.log(1), 0));

            assert(approx_eq(std.sqrt(4), 2));

            assert(std.round(5) == 5);
            assert(std.round(5.12312313) == 5);

            assert(std.ceil(5) == 5);
            assert(std.ceil(5.0001) == 6);

            assert(std.floor(5) == 5);
            assert(std.floor(5.0001) == 5);

            assert(approx_eq(std.sin(std.PI / 2), 1));
            assert(approx_eq(std.cos(std.PI / 3), 0.5));
            assert(approx_eq(std.tan(std.PI / 4), 1));

            assert(approx_eq(std.asin(1), std.PI / 2));
            assert(approx_eq(std.acos(0.5), std.PI / 3));
            assert(approx_eq(std.atan(1), std.PI / 4));
        }

        export func approx_eq(actual, expected) = {
            const a = expected * 0.999;
            const b = expected * 1.001;
            if (a <= b) {
                actual >= a && actual <= b;
            } else {
                actual <= a && actual >= b;
            }
        }
    )";

    eval_test test(source);
    test.call("test").returns_value();
}

} // namespace tiro::eval_tests
