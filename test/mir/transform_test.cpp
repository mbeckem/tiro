#include <catch.hpp>

#include "tiro/compiler/compiler.hpp"
#include "tiro/mir/transform_func.hpp"
#include "tiro/mir/transform_module.hpp"
#include "tiro/mir/types.hpp"

#include "../test_parser.hpp"

using namespace tiro;
using namespace tiro::compiler;

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

TEST_CASE("test mir transform", "[mir]") {
    std::string_view source = R"(
        import std;

        func print(x, y) {
            var z;
            var result = {
                if (x) {
                    z = 1;
                    x;
                } else {
                    z = 2;
                    y;
                }
            };
            result += 1;
            std.print(result);
            return result;
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
    mir::Module mir_module(
        compiler.strings().insert("MODULE_NAME"), compiler.strings());

    mir_transform::ModuleContext ctx(TIRO_NN(module_node.get()), mir_module,
        compiler.diag(), compiler.strings());
    ctx.compile_module();

    PrintStream print;
    dump_module(mir_module, print);
}
