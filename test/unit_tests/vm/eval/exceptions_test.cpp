#include "./test_context.hpp"

#include "support/vm_matchers.hpp"

namespace tiro::vm::test {

using test_support::is_integer_value;

TEST_CASE("Panics should be registered as secondary exceptions if another exception is in flight",
    "[eval]") {
    std::string_view source = R"RAW(
        import std;

        export func test() {
            defer std.panic("test-secondary-1");
            defer nested();
            defer std.panic("test-secondary-2");

            std.panic("test-panic");
        }

        func nested() {
            defer std.panic("nested-secondary-1");
            std.panic("nested-panic");
        }
    )RAW";

    TestContext test(source);

    auto require_message = [&](Value ex, std::string_view expected) {
        REQUIRE(ex.is<Exception>());
        std::string_view message = ex.must_cast<Exception>().message().view();
        CAPTURE(message, expected);

        bool contains_message = message.find(expected) != std::string_view::npos;
        REQUIRE(contains_message);
    };

    Scope sc(test.ctx());

    Local root_exception = sc.local(test.call("test").panics());
    require_message(*root_exception, "test-panic");
    REQUIRE(root_exception->secondary().has_value());

    Local root_secondaries = sc.local(root_exception->secondary().value());
    REQUIRE(root_secondaries->size() == 3);
    require_message(root_secondaries->get(0), "test-secondary-2");
    require_message(root_secondaries->get(1), "nested-panic");
    require_message(root_secondaries->get(2), "test-secondary-1");

    Local nested_exception = sc.local(root_secondaries->get(1).must_cast<Exception>());
    REQUIRE(nested_exception->secondary().has_value());

    Local nested_secondaries = sc.local(nested_exception->secondary().value());
    REQUIRE(nested_secondaries->size() == 1);
    require_message(nested_secondaries->get(0), "nested-secondary-1");
}

} // namespace tiro::vm::test
