#include <catch.hpp>

#include "tiro/compiler/compiler.hpp"
#include "tiro/ir/types.hpp"
#include "tiro/ir_gen/gen_func.hpp"
#include "tiro/ir_gen/gen_module.hpp"

#include "../test_parser.hpp"

using namespace tiro;

[[maybe_unused]] static Ref<FuncDecl>
find_func(const Compiler& comp, std::string_view name) {
    auto interned = comp.strings().find(name);
    REQUIRE(interned);

    auto root = comp.ast_root();
    REQUIRE(root);

    auto file = root->file();
    REQUIRE(file);
    REQUIRE(file->items());

    for (const auto item : file->items()->entries()) {
        auto decl = try_cast<FuncDecl>(item);
        if (decl)
            return ref(decl);
    }

    FAIL("Failed to find function called " << name);
    return {};
}

// TODO: Test compiler output
TEST_CASE("test ir transform", "[ir]") {
    std::string_view source = R"(
        import std;

        func print(z) {
            var x = 0;
            var y = if (z) {
                x = 1;
                3;
            } else {
                x = 2;
                4;
            };
            (z.1, z.a, z[1], x) = f();
            return (x, y);
        }

        func f() {
            return (1, 2);
        }
    )";

    Compiler compiler("test", source);
    if (!compiler.parse() || !compiler.analyze()) {
        for (const auto& message : compiler.diag().messages()) {
            UNSCOPED_INFO(message.text);
        }
        FAIL();
    }

    auto module_node = compiler.ast_root();
    Module module(compiler.strings().insert("MODULE_NAME"), compiler.strings());

    ModuleIRGen ctx(TIRO_NN(module_node.get()), module, compiler.diag(),
        compiler.strings());
    ctx.compile_module();

    // PrintStream print;
    // dump_module(module, print);
}
