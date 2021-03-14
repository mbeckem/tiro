#include <catch2/catch.hpp>

#include "support/test_compiler.hpp"

namespace tiro::test {

TEST_CASE("Using the same record structure multiple times should only generate one record template",
    "[bytecode_gen]") {
    std::string_view source = R"(
        export func a() {
            return (foo: "1", bar: 2, baz: #x);
        }

        export func b() {
            return (foo: 3, bar: 2, baz: 1);
        }

        export func c() {
            return (bar: "x", foo: "y", baz: "z");
        }

        export func d() {
            return (baz: "x", bar: "y", foo: "z");
        }
    )";

    auto module = test_support::compile(source);
    REQUIRE(module->record_count() == 1);
}

} // namespace tiro::test
