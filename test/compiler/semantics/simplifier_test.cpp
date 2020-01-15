#include <catch.hpp>

#include "hammer/compiler/semantics/simplifier.hpp"

#include "../test_parser.hpp"

using namespace hammer;
using namespace compiler;

TEST_CASE("Sequences of string literals should be replaced by a single literal",
    "[simplifier]") {

    TestParser parser;

    SECTION("top level expr string seq") {
        NodePtr<> node;

        node = parser.parse_expr("\"hello\"' world'\"!\"");
        REQUIRE(isa<StringSequenceExpr>(node));

        Simplifier simple(parser.strings(), parser.diag());
        node = simple.simplify(node);
        REQUIRE(!parser.diag().has_errors());
        REQUIRE(isa<StringLiteral>(node));

        auto lit = must_cast<StringLiteral>(node);
        REQUIRE(parser.value(lit->value()) == "hello world!");
    }

    SECTION("in nested context") {
        NodePtr<> root = parser.parse_expr("a = foo(\"hello\"'!', b);");

        Simplifier simple(parser.strings(), parser.diag());
        root = simple.simplify(root);
        REQUIRE(!parser.diag().has_errors());

        auto assign = must_cast<BinaryExpr>(root);
        auto call = must_cast<CallExpr>(assign->right());
        auto lit = must_cast<StringLiteral>(call->args()->get(0));
        REQUIRE(parser.value(lit->value()) == "hello!");
    }
}

TEST_CASE("Interpolated strings should be simplified as well", "[simplifier]") {

    TestParser parser;

    NodePtr<> node = parser.parse_expr(R"RAW(
        $"hello $world!" "!" $" How are you $(doing)?"
    )RAW");
    REQUIRE(isa<StringSequenceExpr>(node));
    REQUIRE(must_cast<StringSequenceExpr>(node)->strings()->size() == 3);

    Simplifier simple(parser.strings(), parser.diag());
    node = simple.simplify(node);
    REQUIRE(!parser.diag().has_errors());
    REQUIRE(isa<InterpolatedStringExpr>(node));

    auto expr = must_cast<InterpolatedStringExpr>(node);
    auto items = must_cast<ExprList>(expr->items());
    REQUIRE(items->size() == 5);

    auto lit1 = must_cast<StringLiteral>(items->get(0));
    REQUIRE(parser.value(lit1->value()) == "hello ");

    auto var1 = must_cast<VarExpr>(items->get(1));
    REQUIRE(parser.value(var1->name()) == "world");

    auto lit2 = must_cast<StringLiteral>(items->get(2));
    REQUIRE(parser.value(lit2->value()) == "!! How are you ");

    auto var2 = must_cast<VarExpr>(items->get(3));
    REQUIRE(parser.value(var2->name()) == "doing");

    auto lit3 = must_cast<StringLiteral>(items->get(4));
    REQUIRE(parser.value(lit3->value()) == "?");
}
