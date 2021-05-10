#include "./test_context.hpp"

#include "support/vm_matchers.hpp"

namespace tiro::vm::test {

using test_support::is_integer_value;

TEST_CASE("User defined code should be able to panic", "[eval]") {
    std::string_view source = R"RAW(
        import std;

        export func test(value) {
            std.panic(value);
        }
    )RAW";

    TestContext test(source);

    Scope sc(test.ctx());
    Local error = sc.local(test.call("test", "my error message").panics());

    REQUIRE(error->is<Exception>());
    auto exception = error.must_cast<Exception>();
    REQUIRE(exception->message().view().find("my error message") != std::string_view::npos);
}

TEST_CASE("Defer statements should run when a function panics", "[eval]") {
    std::string_view source = R"RAW(
        import std;

        export func test(tuple) {
            defer tuple[1] = 2;
            defer tuple[0] = 1;
            std.panic("help!");
        }
    )RAW";

    TestContext test(source);

    Scope sc(test.ctx());
    Local tuple = sc.local(Tuple::make(test.ctx(), 2));
    Local zero = sc.local(test.ctx().get_integer(0));
    tuple->set(0, *zero);
    tuple->set(1, *zero);
    test.call("test", tuple).panics();
    REQUIRE_THAT(tuple->get(0), is_integer_value(1));
    REQUIRE_THAT(tuple->get(1), is_integer_value(2));
}

TEST_CASE("Defer statements should observe variable assignments when a function panics", "[eval]") {
    std::string_view source = R"RAW(
        import std;

        export func test(tuple) {
            var x = 1;
            defer tuple[0] = x;
            no_throw();
            x = 2;
            std.panic("help!");
        }

        func no_throw() {}
    )RAW";

    TestContext test(source);

    Scope sc(test.ctx());
    Local tuple = sc.local(Tuple::make(test.ctx(), 1));
    Local zero = sc.local(test.ctx().get_integer(0));
    tuple->set(0, *zero);
    test.call("test", tuple).panics();
    REQUIRE_THAT(tuple->get(0), is_integer_value(2));
}

TEST_CASE("Defer statements in callers should be executed when a callee panics", "[eval]") {
    std::string_view source = R"RAW(
        import std;

        export func test(array) {
            defer array.append("test1");
            defer array.append("test2");
            a(array);
        }

        func a(array) {
            defer array.append("a");
            b(array);

            defer array.append("NEVER_REACHED (a)");
        }

        func b(array) {
            // b does not have a handler
            c(array);
        }

        func c(array) {
            defer array.append("c");
            std.panic("help!");

            defer array.append("NEVER_REACHED (c)");
        }
    )RAW";

    TestContext test(source);
    Scope sc(test.ctx());
    Local array = sc.local(Array::make(test.ctx(), 99));
    test.call("test", array).panics();

    REQUIRE(array->size() == 4);

    auto test_string = [&](size_t index, std::string_view expected) {
        auto value = array->get(index);
        REQUIRE(value.is<String>());
        REQUIRE(value.must_cast<String>().view() == expected);
    };
    test_string(0, "c");
    test_string(1, "a");
    test_string(2, "test2");
    test_string(3, "test1");
}

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

TEST_CASE("catch_panic should forward normal returns as successful results", "[eval]") {

    std::string_view source = R"RAW(
        import std;

        export func test() {
            return std.catch_panic(func() = 123);
        }
    )RAW";

    TestContext test(source);

    Scope sc(test.ctx());
    Local result = sc.local(test.call("test").returns_value());
    REQUIRE(result->is<Result>());
    REQUIRE(result.must_cast<Result>()->is_success());

    Local value = sc.local(result.must_cast<Result>()->value());
    REQUIRE_THAT(*value, is_integer_value(123));
}

TEST_CASE("catch_panic should forward panics as failed results", "[eval]") {
    std::string_view source = R"RAW(
        import std;

        export func test() {
            return std.catch_panic(do_panic);
        }

        func do_panic() {
            std.panic("help!");
        }
    )RAW";

    TestContext test(source);

    Scope sc(test.ctx());
    Local result = sc.local(test.call("test").returns_value());
    REQUIRE(result->is<Result>());
    REQUIRE(result.must_cast<Result>()->is_failure());

    Local exception = sc.local(result.must_cast<Result>()->reason());
    REQUIRE(exception->is<Exception>());

    std::string_view message = exception.must_cast<Exception>()->message().view();
    bool found = message.find("help!") != std::string_view::npos;
    REQUIRE(found);
}

TEST_CASE("panic should be able to rethrow existing exceptions", "[eval]") {
    std::string_view source = R"RAW(
        import std;

        export func test(ex) {
            std.panic(ex);
        }
    )RAW";

    TestContext test(source);

    Scope sc(test.ctx());
    Local message = sc.local(String::make(test.ctx(), "help!"));
    Local ex = sc.local(Exception::make(test.ctx(), message));
    Local ret = sc.local(test.call("test", ex).panics());
    REQUIRE(ex->same(*ret));
}

} // namespace tiro::vm::test
