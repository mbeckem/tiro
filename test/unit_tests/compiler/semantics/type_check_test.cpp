#include <catch2/catch.hpp>

#include "common/adt/not_null.hpp"
#include "compiler/ast/ast.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/semantics/type_check.hpp"
#include "compiler/semantics/type_table.hpp"

#include "../ast_gen/simple_ast.hpp"

using namespace tiro;

static ExprType expr_type(const TypeTable& types, NotNull<const AstExpr*> expr) {
    return types.get_type(expr->id());
}

TEST_CASE(
    "block expression should have an expression type if their last statement "
    "yields a value",
    "[type-analyzer]") {

    std::string_view source_1 = R"(
        {
            x = 0;
            1;
        }
    )";

    std::string_view source_2 = R"(
        {
            if (x) {
                1;
            } else {
                2;
            }
        }
    )";

    for (auto source : {source_1, source_2}) {
        CAPTURE(source);

        Diagnostics diag;
        auto ast = next::test::parse_expr_ast(source);
        auto node = TIRO_NN(ast.root.get());

        TypeTable types;
        check_types(node, types, diag);
        REQUIRE(!diag.has_errors());
        REQUIRE(expr_type(types, node) == ExprType::Value);
    }
}

TEST_CASE(
    "block expressions without a value-producing statement in their last "
    "position should not have an expression type",
    "[type-analyzer]") {

    std::string_view tests[] = {
        R"(
            {}
        )",
        R"(
            {
                123;
                if (x) {
                    3;
                }
            }
        )",
        R"(
            {
                123;
                {}
            }
        )"};

    for (auto source : tests) {
        CAPTURE(source);

        Diagnostics diag;
        auto ast = next::test::parse_expr_ast(source);
        auto node = TIRO_NN(ast.root.get());

        TypeTable types;
        check_types(node, types, diag);
        REQUIRE(!diag.has_errors());
        REQUIRE(expr_type(types, node) == ExprType::None);
    }
}

TEST_CASE("if expressions should be able to have an expression type", "[type-analyzer]") {

    std::string source = R"(
        if (123) {
            "foo";
        } else {
            {
                "bar";
            }
        }
    )";

    Diagnostics diag;
    auto ast = next::test::parse_expr_ast(source);
    auto node = TIRO_NN(ast.root.get());

    TypeTable types;
    check_types(node, types, diag);
    REQUIRE(!diag.has_errors());
    REQUIRE(expr_type(types, node) == ExprType::Value);
}

TEST_CASE("Expression type should be 'Never' if returning is impossible", "[type-analyzer]") {

    std::string_view tests[] = {
        R"(
            if (1) {
                return 123;
            } else {
                return 456;
            }
        )",
        "return 3", "{ return 'foo'; }", "continue", "break"};

    for (const auto& source : tests) {
        CAPTURE(source);

        Diagnostics diag;
        auto ast = next::test::parse_expr_ast(source);
        auto node = TIRO_NN(ast.root.get());

        TypeTable types;
        check_types(node, types, diag);
        REQUIRE(!diag.has_errors());
        REQUIRE(expr_type(types, node) == ExprType::Never);
    }
}

TEST_CASE("Missing values should raise an error if a value is required", "[type-analyzer]") {
    std::string_view tests[] = {
        R"(
            return {}
        )",
        R"(
            return {
                if (x) {
                    3;
                }
            }
        )",
        R"(
            {
                while ({assert(false);}) {}
            }
        )"};

    for (const auto& source : tests) {
        CAPTURE(source);

        Diagnostics diag;
        auto ast = next::test::parse_expr_ast(source);
        auto node = TIRO_NN(ast.root.get());

        TypeTable types;
        check_types(node, types, diag);
        REQUIRE(diag.has_errors());
    }
}

TEST_CASE("Block expressions used as loop bodies should not need a value", "[type-analyzer]") {
    std::string_view tests[] = {
        R"(
            while true { }
        )",
        R"(
            for var x = 1; x < 10; x += 1 { }
        )",
        R"(
            for item in (1, 2, 3) { }
        )",
    };

    for (const auto& source : tests) {
        CAPTURE(source);

        Diagnostics diag;
        auto ast = next::test::parse_stmt_ast(source);
        auto node = TIRO_NN(ast.root.get());

        TypeTable types;
        check_types(node, types, diag);
        REQUIRE(!diag.has_errors());
    }
}
