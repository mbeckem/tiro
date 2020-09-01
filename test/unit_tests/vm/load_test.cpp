#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/load.hpp"
#include "vm/math.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/module.hpp"
#include "vm/objects/string.hpp"

#include "support/test_compiler.hpp"

using namespace tiro::vm;

TEST_CASE("The module loader must make exported members available", "[load]") {
    auto bytecode_module = tiro::test_compile(R"(
        export func foo(x) {
            return x;
        }

        export const (bar, baz) = (1, 2);

        export const four = foo(foo(foo({
            const a = foo(3);
            const b = (func() { return bar ** 3; })();
            a + b;
        })));

        var not_exported = null;
    )");

    Context ctx;
    Scope sc(ctx);
    Local module = sc.local(load_module(ctx, *bytecode_module));
    REQUIRE(module->name().view() == "test");

    Local exported = sc.local(module->exported());
    REQUIRE(exported->size() == 4);

    auto get_exported = [&](std::string_view name) {
        CAPTURE(name);
        auto found = exported->get(ctx.get_symbol(name));
        REQUIRE(found.has_value());
        return *found;
    };

    auto foo = get_exported("foo");
    REQUIRE(foo.is<Function>());

    auto bar = get_exported("bar");
    REQUIRE(extract_integer(bar) == 1);

    auto baz = get_exported("baz");
    REQUIRE(extract_integer(baz) == 2);

    auto four = get_exported("four");
    REQUIRE(extract_integer(four) == 4);
}
