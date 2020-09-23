#include "support/test_context.hpp"

using namespace tiro;
using namespace tiro::vm;

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
