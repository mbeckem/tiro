#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/math.hpp"
#include "vm/module_registry.hpp"

#include "support/test_compiler.hpp"

using namespace tiro::vm;

TEST_CASE("Module initialization only invokes the initializer once", "[context]") {
    Context ctx;
    Scope sc(ctx);

    // Module to observe init function side effects.
    auto helper_module_compiled = tiro::test_compile_result(
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
    auto test_module_compiled = tiro::test_compile_result(R"(
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
            REQUIRE(extract_integer(*found) == *expected);
        }
    };

    // undefined before initialzier has run.
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
