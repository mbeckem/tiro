#include "support/test_context.hpp"

using namespace tiro::vm;

TEST_CASE("User defined code should be able to panic", "[eval]") {
    std::string_view source = R"RAW(
        import std;

        export func test(value) {
            std.panic(value);
        }
    )RAW";

    // TODO: Exceptions should not be implemented using c++ throw
    TestContext test(source);
    try {
        test.call("test", "my error message").run();
        FAIL("Must have thrown an exception");
    } catch (const tiro::Error& e) {
        std::string_view message = e.what();
        bool found = message.find("my error message") != std::string_view::npos;
        REQUIRE(found);
    } catch (...) {
        FAIL("Unexpected exception type.");
    }
}

TEST_CASE("Defer statements should run when a function panics", "[eval][!mayfail]") {
    std::string_view source = R"RAW(
        import std;

        export func test(tuple) {
            defer tuple[0] = 1;
            std.panic("help!");            
        }
    )RAW";

    TestContext test(source);

    Scope sc(test.ctx());
    Local tuple = sc.local(Tuple::make(test.ctx(), 1));
    Local zero = sc.local(test.ctx().get_integer(0));
    tuple->set(0, *zero);
    REQUIRE_THROWS_AS(test.call("test", tuple).run(), tiro::Error);
    REQUIRE(extract_integer(tuple->get(0)) == 1);
}

TEST_CASE(
    "Defer statements observe variable assignments when a function panics", "[eval][!mayfail]") {
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
    REQUIRE_THROWS_AS(test.call("test", tuple).run(), tiro::Error);
    REQUIRE(extract_integer(tuple->get(0)) == 2);
}
