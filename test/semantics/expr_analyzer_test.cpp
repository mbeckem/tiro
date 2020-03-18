#include <catch.hpp>

#include "tiro/semantics/analyzer.hpp"
#include "tiro/semantics/expr_analyzer.hpp"
#include "tiro/semantics/symbol_table.hpp"
#include "tiro/semantics/type_analyzer.hpp"

#include "../test_parser.hpp"

using namespace tiro;

static void analyze(const NodePtr<>& node) {
    Diagnostics diag;

    TypeAnalyzer types(diag);
    types.dispatch(node, false);
    REQUIRE(!diag.has_errors());

    ExprAnalyzer exprs;
    exprs.dispatch(node, false);
    REQUIRE(!diag.has_errors());
}

TEST_CASE("The body of a function should be observed", "[expr-analyzer]") {
    std::string_view source = R"(
        func foo() {
            3;
        }
    )";

    TestParser parser;
    auto node = parser.parse_toplevel_item(source);
    analyze(node);

    auto func = must_cast<FuncDecl>(node);
    auto body = must_cast<BlockExpr>(func->body());
    REQUIRE(body->expr_type() == ExprType::Value);
    REQUIRE(body->observed());
}

TEST_CASE("Intermediate block expression statements should not be observed",
    "[expr-analyzer]") {
    std::string_view source = R"(
        return {
            a = b + c;
            f();
            4;
        }
    )";

    TestParser parser;
    auto node = parser.parse_expr(source);
    analyze(node);

    auto return_expr = must_cast<ReturnExpr>(node);

    auto block = must_cast<BlockExpr>(return_expr->inner());
    REQUIRE(block->stmts()->size() == 3);

    for (size_t i = 0; i < 2; ++i) {
        auto stmt = must_cast<ExprStmt>(block->stmts()->get(i));
        REQUIRE(!stmt->expr()->observed());
    }

    auto last_stmt = must_cast<ExprStmt>(block->stmts()->get(2));
    REQUIRE(last_stmt->expr()->observed());
}

TEST_CASE(
    "If expression arms should not be observed if the expr does not return a "
    "value",
    "[expr-analyzer]") {
    std::string_view source = R"(
        if (x) {
            a = b;
        }
    )";

    TestParser parser;
    auto node = parser.parse_expr(source);
    analyze(node);

    auto if_expr = must_cast<IfExpr>(node);
    REQUIRE(!can_use_as_value(if_expr));

    auto then_block = must_cast<BlockExpr>(if_expr->then_branch());
    REQUIRE(!then_block->observed());
}

TEST_CASE(
    "If expression arms should not be observed if the expression is not "
    "observed",
    "[expr-analyzer]") {
    std::string_view source = R"(
        return {
            if (a) {
                foo();
            } else {
                bar();
            }
            4;
        };
    )";

    TestParser parser;
    auto node = parser.parse_expr(source);
    analyze(node);

    auto return_expr = must_cast<ReturnExpr>(node);
    auto block_expr = must_cast<BlockExpr>(return_expr->inner());
    REQUIRE(block_expr->stmts()->size() == 2);
    REQUIRE(can_use_as_value(block_expr));
    REQUIRE(block_expr->observed());

    auto if_expr_stmt = must_cast<ExprStmt>(block_expr->stmts()->get(0));

    auto if_expr = must_cast<IfExpr>(if_expr_stmt->expr());
    REQUIRE(can_use_as_value(if_expr));
    REQUIRE(!if_expr->observed());

    auto then_block = must_cast<BlockExpr>(if_expr->then_branch());
    REQUIRE(can_use_as_value(then_block));
    REQUIRE(!then_block->observed());

    auto else_block = must_cast<BlockExpr>(if_expr->else_branch());
    REQUIRE(can_use_as_value(else_block));
    REQUIRE(!else_block->observed());
}

TEST_CASE("Only required loop children should be observed", "[expr-analyzer]") {
    SECTION("for loop") {
        std::string_view source = R"(
            for (var i = 0; i < 10; i = i + 1) {
                i;
                i * 2;
            }
        )";

        TestParser parser;
        auto node = parser.parse_stmt(source);
        analyze(node);

        auto loop = must_cast<ForStmt>(node);
        REQUIRE(can_use_as_value(loop->condition()));
        REQUIRE(loop->condition()->observed());
        REQUIRE(!loop->step()->observed());

        auto body = must_cast<BlockExpr>(loop->body());
        REQUIRE(can_use_as_value(body));
        REQUIRE(!body->observed());
    }

    SECTION("while loop") {
        std::string_view source = R"(
            while (a && b) {
                a;
                b;
            }
        )";

        TestParser parser;
        auto node = parser.parse_stmt(source);
        analyze(node);

        auto loop = must_cast<WhileStmt>(node);
        REQUIRE(loop->condition()->observed());

        auto body = must_cast<BlockExpr>(loop->body());
        REQUIRE(can_use_as_value(body));
        REQUIRE(!body->observed());
    }
}
