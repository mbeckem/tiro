#include <catch.hpp>

#include "hammer/ast/decl.hpp"
#include "hammer/ast/expr.hpp"
#include "hammer/ast/file.hpp"
#include "hammer/ast/literal.hpp"
#include "hammer/ast/stmt.hpp"
#include "hammer/compiler/parser.hpp"

#include <fmt/format.h>

#include <iostream>

using namespace hammer;

template<typename ParseFunc>
auto parse_node(std::string_view source, StringTable& strings, ParseFunc&& fn) {
    Diagnostics diag;
    Parser p("test", source, strings, diag);

    std::unique_ptr node = fn(p);

    CAPTURE(source);

    if (diag.message_count() > 0) {
        for (const auto& msg : diag.messages()) {
            UNSCOPED_INFO(msg.text);
        }

        FAIL();
    }

    REQUIRE(!diag.has_errors());
    REQUIRE(node != nullptr);
    return node;
}

static std::unique_ptr<ast::Expr> parse_expression(
    std::string_view source, StringTable& strings) {
    return parse_node(
        source, strings, [](auto& p) { return p.parse_expr({}); });
}

static std::unique_ptr<ast::Stmt> parse_statement(
    std::string_view source, StringTable& strings) {
    return parse_node(
        source, strings, [](auto& p) { return p.parse_stmt({}); });
}

static std::unique_ptr<ast::File> parse_file(
    std::string_view source, StringTable& strings) {
    return parse_node(source, strings, [](auto& p) { return p.parse_file(); });
}

template<typename T>
static const T* as_node(const ast::Node* node) {
    constexpr ast::NodeKind expected_kind = ast::NodeTypeToKind<T>::value;

    const T* result = try_cast<T>(node);
    INFO("Expected node type: " << ast::to_string(expected_kind));
    INFO("Got node type: " << (node ? ast::to_string(node->kind()) : "null"));
    REQUIRE(result);
    return result;
}

static const ast::BinaryExpr* as_binary(
    const ast::Node* node, ast::BinaryOperator op) {
    const ast::BinaryExpr* result = as_node<ast::BinaryExpr>(node);
    INFO("Expected operation type: " << ast::to_string(op));
    INFO("Got operation type: " << ast::to_string(result->operation()));
    REQUIRE(result->operation() == op);
    return result;
}

static const ast::UnaryExpr* as_unary(
    const ast::Node* node, ast::UnaryOperator op) {
    const ast::UnaryExpr* result = as_node<ast::UnaryExpr>(node);
    INFO("Expected operation type: " << ast::to_string(op));
    INFO("Got operation type: " << ast::to_string(result->operation()));
    REQUIRE(result->operation() == op);
    return result;
}

static const ast::Expr* as_unwrapped_expr(const ast::Node* node) {
    const ast::ExprStmt* result = as_node<ast::ExprStmt>(node);
    REQUIRE(result->expression());
    return result->expression();
}

TEST_CASE("parse arithmetic operator precendence", "[parser]") {
    StringTable strings;
    std::string source = "-4**2 + 1234 * (2.34 - 1)";

    auto expr_result = parse_expression(source, strings);

    const auto* add = as_binary(expr_result.get(), ast::BinaryOperator::Plus);
    const auto* exp = as_binary(add->left_child(), ast::BinaryOperator::Power);
    const auto* unary_minus = as_unary(
        exp->left_child(), ast::UnaryOperator::Minus);

    const auto* unary_child = as_node<ast::IntegerLiteral>(
        unary_minus->inner());
    REQUIRE(unary_child->value() == 4);

    const auto* exp_right = as_node<ast::IntegerLiteral>(exp->right_child());
    REQUIRE(exp_right->value() == 2);

    const auto* mul = as_binary(
        add->right_child(), ast::BinaryOperator::Multiply);

    const auto* mul_left = as_node<ast::IntegerLiteral>(mul->left_child());
    REQUIRE(mul_left->value() == 1234);

    const auto* inner_sub = as_binary(
        mul->right_child(), ast::BinaryOperator::Minus);

    const auto* inner_sub_left = as_node<ast::FloatLiteral>(
        inner_sub->left_child());
    REQUIRE(inner_sub_left->value() == 2.34);

    const auto* inner_sub_right = as_node<ast::IntegerLiteral>(
        inner_sub->right_child());
    REQUIRE(inner_sub_right->value() == 1);
}

TEST_CASE("operator precedence in assignments", "[parser]") {
    StringTable strings;
    std::string source = "a = b = 3 && 4";

    auto expr_result = parse_expression(source, strings);

    const auto* assign_a = as_binary(
        expr_result.get(), ast::BinaryOperator::Assign);

    const auto* var_a = as_node<ast::VarExpr>(assign_a->left_child());
    REQUIRE(strings.value(var_a->name()) == "a");

    const auto* assign_b = as_binary(
        assign_a->right_child(), ast::BinaryOperator::Assign);

    const auto* var_b = as_node<ast::VarExpr>(assign_b->left_child());
    REQUIRE(strings.value(var_b->name()) == "b");

    const auto* binop = as_binary(
        assign_b->right_child(), ast::BinaryOperator::LogicalAnd);

    const auto* lit_3 = as_node<ast::IntegerLiteral>(binop->left_child());
    REQUIRE(lit_3->value() == 3);

    const auto* lit_4 = as_node<ast::IntegerLiteral>(binop->right_child());
    REQUIRE(lit_4->value() == 4);
}

TEST_CASE("parse variable declaration", "[parser]") {
    StringTable strings;
    std::string source = "const i = test();";

    auto decl_result = parse_statement(source, strings);

    const auto* decl = as_node<ast::DeclStmt>(decl_result.get());
    const auto* i_sym = as_node<ast::VarDecl>(decl->declaration());
    REQUIRE(strings.value(i_sym->name()) == "i");

    const auto* init = as_node<ast::CallExpr>(i_sym->initializer());
    REQUIRE(init->arg_count() == 0);

    const auto* func = as_node<ast::VarExpr>(init->func());
    REQUIRE(strings.value(func->name()) == "test");
}

TEST_CASE("parse if statement", "[parser]") {
    StringTable strings;
    std::string source = "if a { return 3; } else if (1) { x; } else { }";

    auto if_result = parse_statement(source, strings);

    const auto* expr = as_node<ast::IfExpr>(
        as_node<ast::ExprStmt>(if_result.get())->expression());

    const auto* var_a = as_node<ast::VarExpr>(expr->condition());
    REQUIRE(strings.value(var_a->name()) == "a");

    const auto* then_block = as_node<ast::BlockExpr>(expr->then_branch());
    REQUIRE(then_block->stmt_count() == 1);

    const auto* ret = as_node<ast::ReturnExpr>(
        as_unwrapped_expr(then_block->get_stmt(0)));
    unused(ret);

    const auto* nested_expr = as_node<ast::IfExpr>(expr->else_branch());

    const auto* int_lit = as_node<ast::IntegerLiteral>(
        nested_expr->condition());
    REQUIRE(int_lit->value() == 1);

    const auto* nested_then_block = as_node<ast::BlockExpr>(
        nested_expr->then_branch());
    REQUIRE(nested_then_block->stmt_count() == 1);

    const auto* var_x = as_node<ast::VarExpr>(
        as_unwrapped_expr(nested_then_block->get_stmt(0)));
    REQUIRE(strings.value(var_x->name()) == "x");

    const auto* else_block = as_node<ast::BlockExpr>(
        nested_expr->else_branch());
    REQUIRE(else_block->stmt_count() == 0);
}

TEST_CASE("parse while statement", "[parser]") {
    StringTable strings;
    std::string source = "while a == b { c; }";

    auto while_result = parse_statement(source, strings);

    const auto* while_stmt = as_node<ast::WhileStmt>(while_result.get());
    const auto* comp = as_binary(
        while_stmt->condition(), ast::BinaryOperator::Equals);

    const auto* lhs = as_node<ast::VarExpr>(comp->left_child());
    REQUIRE(strings.value(lhs->name()) == "a");

    const auto* rhs = as_node<ast::VarExpr>(comp->right_child());
    REQUIRE(strings.value(rhs->name()) == "b");

    const auto* block = as_node<ast::BlockExpr>(while_stmt->body());
    REQUIRE(block->stmt_count() == 1);

    const auto* var = as_node<ast::VarExpr>(
        as_unwrapped_expr(block->get_stmt(0)));
    REQUIRE(strings.value(var->name()) == "c");
}

TEST_CASE("function definition", "[parser]") {
    StringTable strings;
    std::string source = "func myfunc (a, b) { return; }";

    auto file_result = parse_file(source, strings);

    const auto* file = as_node<ast::File>(file_result.get());
    REQUIRE(file->item_count() == 1);

    const auto* func = as_node<ast::FuncDecl>(file->get_item(0));
    REQUIRE(strings.value(func->name()) == "myfunc");
    REQUIRE(func->param_count() == 2);

    const auto* param_a = as_node<ast::ParamDecl>(func->get_param(0));
    REQUIRE(strings.value(param_a->name()) == "a");

    const auto* param_b = as_node<ast::ParamDecl>(func->get_param(1));
    REQUIRE(strings.value(param_b->name()) == "b");

    const auto* body = as_node<ast::BlockExpr>(func->body());
    REQUIRE(body->stmt_count() == 1);

    const auto* ret = as_node<ast::ReturnExpr>(
        as_unwrapped_expr(body->get_stmt(0)));
    REQUIRE(ret->inner() == nullptr);
}

TEST_CASE("block expression", "[parser]") {
    StringTable strings;
    std::string source = "var i = { if (a) { } else { } 4; };";

    auto decl_result = parse_statement(source, strings);

    const auto* decl = as_node<ast::DeclStmt>(decl_result.get());
    const auto* sym = as_node<ast::VarDecl>(decl->declaration());
    REQUIRE(strings.value(sym->name()) == "i");

    const auto* block = as_node<ast::BlockExpr>(sym->initializer());
    REQUIRE(block->stmt_count() == 2);

    const auto* if_expr = as_node<ast::IfExpr>(
        as_node<ast::ExprStmt>(block->get_stmt(0))->expression());
    unused(if_expr);

    const auto* literal = as_node<ast::IntegerLiteral>(
        as_unwrapped_expr(block->get_stmt(1)));
    REQUIRE(literal->value() == 4);
}

TEST_CASE("function calls", "[parser]") {
    StringTable strings;
    std::string source = "f(1)(2, 3)()";

    auto call_result = parse_expression(source, strings);

    const auto* call_1 = as_node<ast::CallExpr>(call_result.get());
    REQUIRE(call_1->arg_count() == 0);

    const auto* call_2 = as_node<ast::CallExpr>(call_1->func());
    REQUIRE(call_2->arg_count() == 2);

    const auto* two = as_node<ast::IntegerLiteral>(call_2->get_arg(0));
    REQUIRE(two->value() == 2);

    const auto* three = as_node<ast::IntegerLiteral>(call_2->get_arg(1));
    REQUIRE(three->value() == 3);

    const auto* call_3 = as_node<ast::CallExpr>(call_2->func());
    REQUIRE(call_3->arg_count() == 1);

    const auto* one = as_node<ast::IntegerLiteral>(call_3->get_arg(0));
    REQUIRE(one->value() == 1);

    const auto* f = as_node<ast::VarExpr>(call_3->func());
    REQUIRE(strings.value(f->name()) == "f");
}

TEST_CASE("dot expressions", "[parser]") {
    StringTable strings;
    std::string source = "a.b.c";

    auto dot_result = parse_expression(source, strings);

    const auto* dot_1 = as_node<ast::DotExpr>(dot_result.get());
    REQUIRE(strings.value(dot_1->name()) == "c");

    const auto* dot_2 = as_node<ast::DotExpr>(dot_1->inner());
    REQUIRE(strings.value(dot_2->name()) == "b");

    const auto* var = as_node<ast::VarExpr>(dot_2->inner());
    REQUIRE(strings.value(var->name()) == "a");
}
