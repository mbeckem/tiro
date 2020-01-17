#include <catch.hpp>

#include "tiro/compiler/semantics/type_analyzer.hpp"

#include "../test_parser.hpp"

using namespace tiro;
using namespace compiler;

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

        TestParser parser;
        auto node = parser.parse_expr(source);

        TypeAnalyzer types(parser.diag());
        types.dispatch(node, false);

        REQUIRE(!parser.diag().has_errors());
        REQUIRE(node->expr_type() == ExprType::Value);
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

        TestParser parser;
        auto node = parser.parse_expr(source);

        TypeAnalyzer types(parser.diag());
        types.dispatch(node, false);

        REQUIRE(!parser.diag().has_errors());
        REQUIRE(node->expr_type() == ExprType::None);
    }
}

TEST_CASE("if expressions should be able to have an expression type",
    "[type-analyzer]") {

    std::string source = R"(
        if (123) {
            "foo";
        } else {
            {
                "bar";
            }
        }
    )";

    TestParser parser;
    auto node = parser.parse_expr(source);

    TypeAnalyzer types(parser.diag());
    types.dispatch(node, false);

    REQUIRE(!parser.diag().has_errors());
    REQUIRE(node->expr_type() == ExprType::Value);
}

TEST_CASE("Expression type should be 'Never' if returning is impossible",
    "[type-analyzer]") {

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

        TestParser parser;
        auto node = parser.parse_expr(source);

        TypeAnalyzer types(parser.diag());
        types.dispatch(node, false);

        REQUIRE(!parser.diag().has_errors());
        REQUIRE(node->expr_type() == ExprType::Never);
    }
}

TEST_CASE("Missing values should raise an error if a value is required",
    "[type-analyzer]") {
    std::string_view tests[] = {
        R"(
            return {};
        )",
        R"(
            return {
                if (x) {
                    3;
                }
            };
        )",
        R"(
            {
                while ({assert(false);}) {}
            }
        )"};

    for (const auto& source : tests) {
        CAPTURE(source);

        TestParser parser;
        auto node = parser.parse_expr(source);

        TypeAnalyzer types(parser.diag());
        types.dispatch(node, false);

        REQUIRE(parser.diag().has_errors());
    }
}
