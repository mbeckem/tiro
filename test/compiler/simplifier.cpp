#include <catch.hpp>

#include "hammer/compiler/semantics/simplifier.hpp"

#include "./test_parser.hpp"

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
