#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/math.hpp"
#include "vm/module_loader.hpp"
#include "vm/objects/hash_table.hpp"
#include "vm/objects/module.hpp"
#include "vm/objects/string.hpp"

#include "support/test_compiler.hpp"
#include "support/vm_matchers.hpp"

namespace tiro::vm::test {

using test_support::is_integer_value;

TEST_CASE("The module loader must make exported members available", "[module-loader]") {
    auto bytecode_module = test_support::compile(R"(
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
    REQUIRE_FALSE(module->initialized());

    ctx.modules().resolve_module(ctx, module);
    REQUIRE(module->initialized());

    Local exported = sc.local(module->exported());
    REQUIRE(exported->size() == 4);

    auto get_exported = [&](std::string_view name) {
        CAPTURE(name);

        auto symbol = ctx.get_symbol(name);
        auto found = module->find_exported(symbol);
        REQUIRE(found);
        return *found;
    };

    auto foo = get_exported("foo");
    REQUIRE(foo.is<CodeFunction>());

    auto bar = get_exported("bar");
    REQUIRE_THAT(bar, is_integer_value(1));

    auto baz = get_exported("baz");
    REQUIRE_THAT(baz, is_integer_value(2));

    auto four = get_exported("four");
    REQUIRE_THAT(four, is_integer_value(4));
}

} // namespace tiro::vm::test
