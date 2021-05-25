#include "./test_context.hpp"

namespace tiro::vm::test {

TEST_CASE("String and StringSlice should support common methods", "[eval]") {
    std::string_view source = R"RAW(
        import std;

        const factories = [create_string, create_slice];

        export func test() {
            for factory in factories {
                test_equals(factory);
                test_contains(factory);
                test_size(factory);
                test_slice(factory);
            }
        }

        export func test_equals(factory) {
            const a = factory("a");
            assert(a == a);
            assert(!(a != a));

            for other in factories {
                assert(a == other("a"));
                assert(a != other("b"));
            }
        }

        export func test_contains(factory) {
            const s = factory("xyzfooxyz");

            for other in factories {
                assert(s.contains(other("")));
                assert(s.contains(other("foo")));
                assert(!s.contains(other("fooy")));
                assert(!s.contains(other("unrelated")));
            }
        }

        export func test_slice(factory) {
            const s1 = factory("foobarbaz").slice(3, 2);
            assert(std.type_of(s1) == std.StringSlice);
            assert(s1.size() == 2);
            assert(s1 == "ba");

            const s2 = factory("foobarbaz").slice_first(3);
            assert(std.type_of(s2) == std.StringSlice);
            assert(s2.size() == 3);
            assert(s2 == "foo");

            const s3 = factory("foobarbaz").slice_last(4);
            assert(std.type_of(s3) == std.StringSlice);
            assert(s3.size() == 4);
            assert(s3 == "rbaz");

            const s4 = factory("xyz").slice_first(9999);
            assert(s4 == "xyz");

            const s5 = factory("xyz").slice_last(9999);
            assert(s5 == "xyz");

            const s6 = factory("xyz").slice(9, 10);
            assert(s6 == "");

            const s7 = factory("xyz").slice(1, 9999);
            assert(s7 == "yz");
        }

        export func test_size(factory) {
            assert(factory("").size() == 0);
            assert(factory("foo").size() == 3);
        }

        func create_string(content) {
            assert(std.type_of(content) == std.String);
            return content;
        }

        func create_slice(content) {
            assert(std.type_of(content) == std.String);
            const slice = content.slice(0, content.size());
            assert(std.type_of(slice) == std.StringSlice);
            return slice;
        }

    )RAW";

    TestContext test(source);
    test.call("test").returns_null();
}

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

TEST_CASE("String should support iteration", "[eval]") {
    std::string_view source = R"RAW(
        import std;

        export func tokenize(str) {
            const builder = std.new_string_builder();
            var index = 0;
            for char in str {
                if index > 0 {
                    builder.append(",");
                }
                index += 1;
                builder.append(char);
            }
            return builder.to_string();
        }

        export func tokenize_slice(str, start, length) {
            return tokenize(str.slice(start, length));
        }
    )RAW";

    TestContext test(source);
    test.call("tokenize", "abcde").returns_string("a,b,c,d,e");
    test.call("tokenize_slice", "foobar", 2, 3).returns_string("o,b,a");
}

} // namespace tiro::vm::test
