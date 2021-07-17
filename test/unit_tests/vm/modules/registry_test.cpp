#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/math.hpp"
#include "vm/modules/load.hpp"
#include "vm/modules/registry.hpp"

#include "support/matchers.hpp"
#include "support/test_compiler.hpp"
#include "support/vm_matchers.hpp"

namespace tiro::vm::test {

using test_support::compile_result;
using test_support::is_integer_value;

TEST_CASE("Module initialization only invokes the initializer once", "[module-registry]") {
    Context ctx;
    Scope sc(ctx);

    // Module to observe init function side effects.
    auto helper_module_compiled = compile_result(
        R"(
            var i = 1;

            export func side_effect() {
                return i += 1;
            }
        )",
        "helper");
    Local helper_module = sc.local(load_module(ctx, *helper_module_compiled.module));
    ctx.modules().add_module(ctx, helper_module);

    // Init function calls the side effect function.
    auto test_module_compiled = compile_result(R"(
        import helper;

        export const value = helper.side_effect();
    )");
    Local test_module = sc.local(load_module(ctx, *test_module_compiled.module));

    Local value_symbol = sc.local(ctx.get_symbol("value"));
    auto assert_value = [&](std::optional<int> expected) {
        auto found = test_module->find_exported(*value_symbol);
        REQUIRE(found);
        if (!expected) {
            REQUIRE(found->is<Undefined>());
        } else {
            REQUIRE_THAT(*found, is_integer_value(*expected));
        }
    };

    // undefined before initializer has run.
    REQUIRE(!test_module->initialized());
    assert_value({});

    // Resolving triggers call of init function.
    ctx.modules().resolve_module(ctx, test_module);
    REQUIRE(test_module->initialized());
    assert_value(2);

    // No change on repeated calls.
    ctx.modules().resolve_module(ctx, test_module);
    assert_value(2);
}

TEST_CASE(
    "Module dependency cycles should be detected during module resolution", "[module-registry]") {
    Context ctx;
    Scope sc(ctx);

    auto foo_compiled = compile_result(
        R"(
            import bar;
        )",
        "foo");
    Local foo_module = sc.local(load_module(ctx, *foo_compiled.module));
    ctx.modules().add_module(ctx, foo_module);

    auto bar_compiled = compile_result(
        R"(
            import baz;
        )",
        "bar");
    Local bar_module = sc.local(load_module(ctx, *bar_compiled.module));
    ctx.modules().add_module(ctx, bar_module);

    auto baz_compiled = compile_result(
        R"(
            import foo;
        )",
        "baz");
    Local baz_module = sc.local(load_module(ctx, *baz_compiled.module));
    ctx.modules().add_module(ctx, baz_module);

    REQUIRE_THROWS_MATCHES(ctx.modules().resolve_module(ctx, foo_module), tiro::Error,
        test_support::exception_contains_string(
            "module foo is part of a forbidden dependency cycle"));
}

} // namespace tiro::vm::test
