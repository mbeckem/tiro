#include "support/test_context.hpp"

#include "vm/handles/scope.hpp"

using namespace tiro;
using namespace tiro::vm;

TEST_CASE("Records should be constructible through syntax", "[eval]") {
    std::string_view source = R"(
        export func test() {
            return (foo: "x", bar: 3);
        }
    )";

    TestContext test(source);

    auto result = test.call("test").run();
    REQUIRE(result->is<vm::Record>());

    {
        vm::Scope sc(test.ctx());
        auto rec = result.must_cast<vm::Record>();
        vm::Local keys = sc.local(vm::Record::keys(test.ctx(), rec));
        REQUIRE(keys->size() == 2);

        vm::Local foo = sc.local(test.ctx().get_symbol("foo"));
        auto foo_value = rec->get(*foo);
        REQUIRE(foo_value);
        REQUIRE(foo_value->is<String>());
        REQUIRE(foo_value->must_cast<String>().view() == "x");

        vm::Local bar = sc.local(test.ctx().get_symbol("bar"));
        auto bar_value = rec->get(*bar);
        REQUIRE(bar_value);
        REQUIRE(extract_integer(*bar_value) == 3);
    }
}

TEST_CASE("Record's members should be inspectable and modifiable", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test_record() {
            const rec = std.new_record([#foo]);
            rec.foo = 3;
            return rec.foo * -1;
        }
    )";

    TestContext test(source);
    test.call("test_record").returns_int(-3);
}

TEST_CASE("Record's members should be null by default", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test_record() = {
            const rec = std.new_record([#foo]);
            rec.foo;
        }
    )";

    TestContext test(source);
    test.call("test_record").returns_null();
}

TEST_CASE("Record's member functions should be invokable", "[eval]") {
    std::string_view source = R"(
        import std;

        export func test_record() = {
            const rec = std.new_record([#function]);
            rec.function = func(x) = {
                x * 2;
            };
            rec.function(3);
        }
    )";

    TestContext test(source);
    test.call("test_record").returns_int(6);
}
