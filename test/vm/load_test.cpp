#include <catch.hpp>

#include "vm/context.hpp"
#include "vm/load.hpp"
#include "vm/math.hpp"
#include "vm/objects/hash_tables.hpp"
#include "vm/objects/modules.hpp"
#include "vm/objects/strings.hpp"

#include "support/test_compiler.hpp"

using namespace tiro;

TEST_CASE("The module loader must make exported members available", "[load]") {
    auto bytecode_module = test_compile(R"(
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

    vm::Context ctx;
    vm::Root<vm::Module> module(ctx, vm::load_module(ctx, *bytecode_module));
    REQUIRE(module->name().view() == "test");

    vm::Root<vm::HashTable> exported(ctx, module->exported());
    REQUIRE(exported->size() == 4);

    auto get_exported = [&](std::string_view name) {
        CAPTURE(name);

        vm::Root symbol(ctx, ctx.get_symbol(name));
        auto found = exported->get(symbol);
        REQUIRE(found.has_value());
        return *found;
    };

    auto foo = get_exported("foo");
    REQUIRE(foo.is<vm::Function>());

    auto bar = get_exported("bar");
    REQUIRE(vm::extract_integer(bar) == 1);

    auto baz = get_exported("baz");
    REQUIRE(vm::extract_integer(baz) == 2);

    auto four = get_exported("four");
    REQUIRE(vm::extract_integer(four) == 4);
}
