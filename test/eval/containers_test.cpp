#include "support/test_context.hpp"

using namespace tiro::vm;

TEST_CASE("Array size should be returned correctly", "[eval]") {
    std::string_view source = R"RAW(
        export func test_initial() {
            var array = [1, 2, 3, 4, 5];
            return array.size();
        }

        export func test_empty() {
            return [].size();
        }

        export func test_append() {
            var array = [1, 2];
            array.append("foo");
            return array.size();
        }
    )RAW";

    TestContext test(source);
    test.call("test_initial").returns_int(5);
    test.call("test_empty").returns_int(0);
    test.call("test_append").returns_int(3);
}

TEST_CASE("Array data should be accessible", "[eval]") {
    std::string_view source = R"(
        import std;

        export func get(index) {
            return [1, 2, 3, 4][index];
        }

        export func set(index, value) {
            const x = [1, 2, 3, 4];
            x[index] = value;
            return x[index];
        }
    )";

    TestContext test(source);
    test.call("get", 0).returns_int(1);
    test.call("get", 2).returns_int(3);
    test.call("set", 3, 123).returns_int(123);
}

TEST_CASE("Array should support iteration", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test() {
            const array = [1, 2, 3, 4, 5];
            const builder = std.new_string_builder();
            for item in array {
                builder.append(item);
            }
            return builder.to_string();
        }
    )";

    TestContext test(source);
    test.call("test").returns_string("12345");
}

TEST_CASE("Buffer data should be accessible", "[eval]") {
    std::string_view source = R"(
        import std;

        export func buffer_size() {
            const b = std.new_buffer(1234);
            return b.size();
        }

        export func buffer_get() = {
            const b = std.new_buffer(4096);
            b[4095];
        }

        export func buffer_set() {
            const b = std.new_buffer(4096);
            b[123] = 64;
            return b[123];
        }
    )";

    TestContext test(source);
    test.call("buffer_size").returns_int(1234);
    test.call("buffer_get").returns_int(0);
    test.call("buffer_set").returns_int(64);
}

TEST_CASE("Tuple members should be accessible", "[eval]") {
    std::string_view source = R"(
        export func tuple_members() {
            var tup = (1, (2, 3));
            tup.1.0 = 4;
            return tup.1.0;
        }
    )";

    TestContext test(source);
    auto result = test.run("tuple_members");
    REQUIRE(extract_integer(*result) == 4);
}

TEST_CASE("Tuple size should be returned correctly", "[eval]") {
    std::string_view source = R"RAW(
        export func test_size() {
            const tuple = (1, 2, 3);
            return tuple.size();
        }

        export func test_empty() {
            return ().size();
        }
    )RAW";

    TestContext test(source);

    {
        auto result = test.run("test_size");
        REQUIRE(extract_integer(*result) == 3);
    }

    {
        auto result = test.run("test_empty");
        REQUIRE(extract_integer(*result) == 0);
    }
}

TEST_CASE("Tuples should support iteration", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test() {
            const tuple = (1, 2, 3, 4, 5);
            const builder = std.new_string_builder();
            for item in tuple {
                builder.append(item);
            }
            return builder.to_string();
        }
    )";

    TestContext test(source);
    test.call("test").returns_string("12345");
}

TEST_CASE("Methods of the map class should be callable", "[eval]") {
    std::string_view source = R"(
        export func map_usage() {
            const m = map{
                "key": "value",
                "rm": null,
            };
            m[1] = 2;
            m["key"] = "key";
            m[null] = 3;

            m.remove("rm");
            m[1] = m.contains(1);
            m[null] = m.contains("other_key");
            
            return m;
        }
    )";

    TestContext test(source);
    auto result = test.call("map_usage").run();
    REQUIRE(result->is<HashTable>());

    auto table = result.handle().must_cast<HashTable>();
    REQUIRE(table->size() == 3);

    Context& ctx = test.ctx();

    // "key"
    {
        Scope sc(ctx);
        Local key = sc.local(String::make(ctx, "key"));
        REQUIRE(table->contains(*key));

        Local value = sc.local(Value::null());
        if (auto found = table->get(*key))
            value.set(*found);

        REQUIRE(value->is<String>());
        REQUIRE(value->must_cast<String>().view() == "key");
    }

    // null
    {
        Scope sc(ctx);
        Local value = sc.local(Value::null());
        if (auto found = table->get(Value::null()); found)
            value.set(*found);

        REQUIRE(value->same(ctx.get_boolean(false)));
    }

    // 1
    {
        Scope sc(ctx);
        Local key = sc.local(ctx.get_integer(1));
        Local value = sc.local(Value::null());
        if (auto found = table->get(*key); found)
            value.set(*found);

        REQUIRE(value->same(ctx.get_boolean(true)));
    }
}

TEST_CASE("Maps should support iteration in insertion order", "[eval]") {
    std::string_view source = R"(
        import std;

        func make_map() = {
            const map = map{
                "qux": "0",
                "foo": "1",
                "bar": "-1",
                "baz": "3",
            };
            map.remove("qux");
            map["qux"] = 4; // Reinsertion
            map["bar"] = 2; // Update does not change order
            map;
        }

        export func test_entries() {
            const map = make_map();
            const builder = std.new_string_builder();
            var first = true;
            for (key, value) in map {
                if (first) {
                    first = false;
                } else {
                    builder.append(",");
                }
                builder.append(key, ":", value);
            }
            return builder.to_string();
        }

        export func test_keys() {
            const map = make_map();
            const builder = std.new_string_builder();
            var first = true;
            for key in map.keys() {
                if (first) {
                    first = false;
                } else {
                    builder.append(",");
                }
                builder.append(key);
            }
            return builder.to_string();
        }

        export func test_values() {
            const map = make_map();
            const builder = std.new_string_builder();
            for value in map.values() {
                builder.append(value);
            }
            return builder.to_string();
        }
    )";

    TestContext test(source);
    test.call("test_entries").returns_string("foo:1,bar:2,baz:3,qux:4");
    test.call("test_keys").returns_string("foo,bar,baz,qux");
    test.call("test_values").returns_string("1234");
}
