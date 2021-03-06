#include <catch2/catch.hpp>

#include "compiler/ast/casting.hpp"
#include "compiler/ast/expr.hpp"
#include "compiler/ast_gen/build_ast.hpp"

#include "./simple_ast.hpp"

using namespace tiro;
using namespace tiro::next;
using namespace tiro::next::test;

template<typename T, typename U>
static NotNull<T*> check(U* ptr) {
    INFO("Actual type: " << (ptr ? to_string(ptr->type()) : "<NULL>"sv));
    REQUIRE(is_instance<T>(ptr));
    return must_cast<T>(ptr);
}

TEST_CASE("ast should support null literals", "[ast-gen]") {
    auto ast = parse_expr_ast("null");
    check<AstNullLiteral>(ast.root.get());
}

TEST_CASE("ast should support boolean literals", "[ast-gen]") {
    auto true_ast = parse_expr_ast("true");
    auto true_as_boolean = check<AstBooleanLiteral>(true_ast.root.get());
    REQUIRE(true_as_boolean->value() == true);

    auto false_ast = parse_expr_ast("false");
    auto false_as_boolean = check<AstBooleanLiteral>(false_ast.root.get());
    REQUIRE(false_as_boolean->value() == false);
}

TEST_CASE("ast should support symbol literals", "[ast-gen]") {
    auto ast = parse_expr_ast("#symbol_123");
    auto symbol = check<AstSymbolLiteral>(ast.root.get());
    REQUIRE(ast.strings.value(symbol->value()) == "symbol_123");
}

TEST_CASE("ast should support integer literals", "[ast-gen]") {
    struct Test {
        std::string_view source;
        i64 expected;
    };

    Test tests[] = {
        {"123", i64(123)},
        {"0x123", i64(0x123)},
        {"0o123", i64(0123)},
        {"0b01001", i64(9)},
        {"1___2___3", i64(123)},
    };

    for (const auto& test : tests) {
        CAPTURE(test.source);
        CAPTURE(test.expected);

        auto ast = parse_expr_ast(test.source);
        auto literal = check<AstIntegerLiteral>(ast.root.get());
        REQUIRE(literal->value() == test.expected);
    }
}

TEST_CASE("ast should support float literals", "[ast-gen]") {
    struct Test {
        std::string_view source;
        double expected;
    };

    Test tests[] = {
        {"123.4", double(123.4)},
        {"123.10101", double(123.10101)},
        {"1_2_3.4_5", double(123.45)},
        {"1_____.____2____", double(1.2)},
    };

    for (const auto& test : tests) {
        CAPTURE(test.source);
        CAPTURE(test.expected);

        auto ast = parse_expr_ast(test.source);
        auto literal = check<AstFloatLiteral>(ast.root.get());
        REQUIRE(literal->value() == test.expected);
    }
}

TEST_CASE("ast should support binary operators", "[ast-gen]") {
    auto ast = parse_expr_ast("1 + 2");
    auto binary = check<AstBinaryExpr>(ast.root.get());
    REQUIRE(binary->operation() == BinaryOperator::Plus);

    auto lhs = check<AstIntegerLiteral>(binary->left());
    REQUIRE(lhs->value() == 1);

    auto rhs = check<AstIntegerLiteral>(binary->right());
    REQUIRE(rhs->value() == 2);
}

TEST_CASE("ast should support unary operators", "[ast-gen]") {
    auto ast = parse_expr_ast("-3");
    auto unary = check<AstUnaryExpr>(ast.root.get());
    REQUIRE(unary->operation() == UnaryOperator::Minus);

    auto operand = check<AstIntegerLiteral>(unary->inner());
    REQUIRE(operand->value() == 3);
}

TEST_CASE("ast should support variable expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("abc");
    auto var = check<AstVarExpr>(ast.root.get());
    REQUIRE(ast.strings.value(var->name()) == "abc");
}

TEST_CASE("ast should unwrap grouped expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("(1)");
    auto expr = check<AstIntegerLiteral>(ast.root.get());
    REQUIRE(expr->value() == 1);
}

TEST_CASE("ast should support break expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("break");
    check<AstBreakExpr>(ast.root.get());
}

TEST_CASE("ast should support continue expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("continue");
    check<AstContinueExpr>(ast.root.get());
}

TEST_CASE("ast should support return expressions without a value", "[ast-gen]") {
    auto ast = parse_expr_ast("return");
    auto ret = check<AstReturnExpr>(ast.root.get());
    REQUIRE(ret->value() == nullptr);
}

TEST_CASE("ast should support return expressions with a value", "[ast-gen]") {
    auto ast = parse_expr_ast("return 4");
    auto ret = check<AstReturnExpr>(ast.root.get());
    auto lit = check<AstIntegerLiteral>(ret->value());
    REQUIRE(lit->value() == 4);
}

TEST_CASE("ast should support instance property expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("a.b");
    auto prop = check<AstPropertyExpr>(ast.root.get());
    REQUIRE(prop->access_type() == AccessType::Normal);

    auto instance = check<AstVarExpr>(prop->instance());
    REQUIRE(ast.strings.value(instance->name()) == "a");

    auto field = check<AstStringIdentifier>(prop->property());
    REQUIRE(ast.strings.value(field->value()) == "b");
}

TEST_CASE("ast should support tuple field expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("a.0");
    auto prop = check<AstPropertyExpr>(ast.root.get());
    REQUIRE(prop->access_type() == AccessType::Normal);

    auto instance = check<AstVarExpr>(prop->instance());
    REQUIRE(ast.strings.value(instance->name()) == "a");

    auto field = check<AstNumericIdentifier>(prop->property());
    REQUIRE(field->value() == 0);
}

TEST_CASE("ast should support optional field access expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("a?.b");
    auto prop = check<AstPropertyExpr>(ast.root.get());
    REQUIRE(prop->access_type() == AccessType::Optional);

    auto instance = check<AstVarExpr>(prop->instance());
    REQUIRE(ast.strings.value(instance->name()) == "a");

    auto field = check<AstStringIdentifier>(prop->property());
    REQUIRE(ast.strings.value(field->value()) == "b");
}

TEST_CASE("ast should support element expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("a[1]");
    auto expr = check<AstElementExpr>(ast.root.get());
    REQUIRE(expr->access_type() == AccessType::Normal);

    auto instance = check<AstVarExpr>(expr->instance());
    REQUIRE(ast.strings.value(instance->name()) == "a");

    auto element = check<AstIntegerLiteral>(expr->element());
    REQUIRE(element->value() == 1);
}

TEST_CASE("ast should support optional element expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("a?[1]");
    auto expr = check<AstElementExpr>(ast.root.get());
    REQUIRE(expr->access_type() == AccessType::Optional);

    auto instance = check<AstVarExpr>(expr->instance());
    REQUIRE(ast.strings.value(instance->name()) == "a");

    auto element = check<AstIntegerLiteral>(expr->element());
    REQUIRE(element->value() == 1);
}
