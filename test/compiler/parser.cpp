#include <catch.hpp>

#include "hammer/compiler/syntax/ast.hpp"
#include "hammer/compiler/syntax/parser.hpp"

#include <fmt/format.h>

using namespace hammer;
using namespace hammer::compiler;

template<typename ParseFunc>
auto parse_node(std::string_view source, StringTable& strings, ParseFunc&& fn) {
    Diagnostics diag;
    Parser p("test", source, strings, diag);

    Parser::Result result = fn(p);

    CAPTURE(source);

    if (diag.message_count() > 0) {
        for (const auto& msg : diag.messages()) {
            UNSCOPED_INFO(msg.text);
        }

        FAIL();
    }

    REQUIRE(!diag.has_errors());
    REQUIRE(result);
    REQUIRE(result.has_node());
    return result.take_node();
}

static NodePtr<Expr>
parse_expression(std::string_view source, StringTable& strings) {
    return parse_node(
        source, strings, [](auto& p) { return p.parse_expr({}); });
}

static NodePtr<Stmt>
parse_statement(std::string_view source, StringTable& strings) {
    return parse_node(
        source, strings, [](auto& p) { return p.parse_stmt({}); });
}

static NodePtr<File> parse_file(std::string_view source, StringTable& strings) {
    return parse_node(source, strings, [](auto& p) { return p.parse_file(); });
}

template<typename T>
static NodePtr<T> as_node(const NodePtr<>& node) {
    constexpr auto expected_type = NodeTraits<T>::node_type;

    const auto result = try_cast<T>(node);
    INFO("Expected node type: " << to_string(expected_type));
    INFO("Got node type: " << (node ? to_string(node->type()) : "null"));
    REQUIRE(result);
    return result;
}

static NodePtr<BinaryExpr> as_binary(const NodePtr<>& node, BinaryOperator op) {
    const auto result = as_node<BinaryExpr>(node);
    INFO("Expected operation type: " << to_string(op));
    INFO("Got operation type: " << to_string(result->operation()));
    REQUIRE(result->operation() == op);
    return result;
}

static NodePtr<UnaryExpr> as_unary(const NodePtr<>& node, UnaryOperator op) {
    const auto result = as_node<UnaryExpr>(node);
    INFO("Expected operation type: " << to_string(op));
    INFO("Got operation type: " << to_string(result->operation()));
    REQUIRE(result->operation() == op);
    return result;
}

static NodePtr<Expr> as_unwrapped_expr(const NodePtr<>& node) {
    const auto result = as_node<ExprStmt>(node);
    REQUIRE(result->expr());
    return result->expr();
}

TEST_CASE("Parser should respect arithmetic operator precendence", "[parser]") {
    StringTable strings;
    std::string source = "-4**2 + 1234 * (2.34 - 1)";

    auto expr_result = parse_expression(source, strings);

    const auto add = as_binary(expr_result, BinaryOperator::Plus);
    const auto exp = as_binary(add->left(), BinaryOperator::Power);
    const auto unary_minus = as_unary(exp->left(), UnaryOperator::Minus);

    const auto unary_child = as_node<IntegerLiteral>(unary_minus->inner());
    REQUIRE(unary_child->value() == 4);

    const auto exp_right = as_node<IntegerLiteral>(exp->right());
    REQUIRE(exp_right->value() == 2);

    const auto mul = as_binary(add->right(), BinaryOperator::Multiply);

    const auto mul_left = as_node<IntegerLiteral>(mul->left());
    REQUIRE(mul_left->value() == 1234);

    const auto inner_sub = as_binary(mul->right(), BinaryOperator::Minus);

    const auto inner_sub_left = as_node<FloatLiteral>(inner_sub->left());
    REQUIRE(inner_sub_left->value() == 2.34);

    const auto inner_sub_right = as_node<IntegerLiteral>(inner_sub->right());
    REQUIRE(inner_sub_right->value() == 1);
}

TEST_CASE(
    "Parser should support operator precedence in assignments", "[parser]") {
    StringTable strings;
    std::string source = "a = b = 3 && 4";

    auto expr_result = parse_expression(source, strings);

    const auto assign_a = as_binary(expr_result, BinaryOperator::Assign);

    const auto var_a = as_node<VarExpr>(assign_a->left());
    REQUIRE(strings.value(var_a->name()) == "a");

    const auto assign_b = as_binary(assign_a->right(), BinaryOperator::Assign);

    const auto var_b = as_node<VarExpr>(assign_b->left());
    REQUIRE(strings.value(var_b->name()) == "b");

    const auto binop = as_binary(assign_b->right(), BinaryOperator::LogicalAnd);

    const auto lit_3 = as_node<IntegerLiteral>(binop->left());
    REQUIRE(lit_3->value() == 3);

    const auto lit_4 = as_node<IntegerLiteral>(binop->right());
    REQUIRE(lit_4->value() == 4);
}

TEST_CASE("Parser should group successive strings in a list", "[parser]") {
    StringTable strings;

    SECTION("normal string is not grouped") {
        auto node = parse_expression("\"hello world\"", strings);
        auto string = as_node<StringLiteral>(node);
        REQUIRE(strings.value(string->value()) == "hello world");
    }

    SECTION("successive strings are grouped") {
        auto node = parse_expression("\"hello\" \" world\"", strings);
        auto sequence = as_node<StringSequenceExpr>(node);
        auto list = as_node<ExprList>(sequence->strings());
        REQUIRE(list->size() == 2);

        auto first = as_node<StringLiteral>(list->get(0));
        REQUIRE(strings.value(first->value()) == "hello");

        auto second = as_node<StringLiteral>(list->get(1));
        REQUIRE(strings.value(second->value()) == " world");
    }
}

TEST_CASE("Parser should recognize assert statements", "[parser]") {
    StringTable strings;

    SECTION("form with one argument") {
        std::string source = "assert(true);";

        auto stmt_result = parse_statement(source, strings);

        const auto stmt = as_node<AssertStmt>(stmt_result);
        const auto true_lit = as_node<BooleanLiteral>(stmt->condition());
        REQUIRE(true_lit->value() == true);
        REQUIRE(stmt->message() == nullptr);
    }

    SECTION("form with two arguements") {
        std::string source = "assert(123, \"error message\");";

        auto stmt_result = parse_statement(source, strings);

        const auto stmt = as_node<AssertStmt>(stmt_result);

        const auto int_lit = as_node<IntegerLiteral>(stmt->condition());
        REQUIRE(int_lit->value() == 123);

        const auto str_lit = as_node<StringLiteral>(stmt->message());
        REQUIRE(strings.value(str_lit->value()) == "error message");
    }
}

TEST_CASE("Parser should recognize constant declarations", "[parser]") {
    StringTable strings;
    std::string source = "const i = test();";

    auto decl_result = parse_statement(source, strings);

    const auto stmt = as_node<DeclStmt>(decl_result);
    const auto i_sym = as_node<VarDecl>(stmt->decl());
    REQUIRE(strings.value(i_sym->name()) == "i");

    const auto init = as_node<CallExpr>(i_sym->initializer());
    const auto args = as_node<ExprList>(init->args());
    REQUIRE(args->size() == 0);

    const auto func = as_node<VarExpr>(init->func());
    REQUIRE(strings.value(func->name()) == "test");
}

TEST_CASE("Parser should recognize if statements", "[parser]") {
    StringTable strings;
    std::string source = "if a { return 3; } else if (1) { x; } else { }";

    auto if_result = parse_statement(source, strings);

    const auto expr = as_node<IfExpr>(as_node<ExprStmt>(if_result)->expr());

    const auto var_a = as_node<VarExpr>(expr->condition());
    REQUIRE(strings.value(var_a->name()) == "a");

    const auto then_block = as_node<BlockExpr>(expr->then_branch());
    const auto then_stmts = as_node<StmtList>(then_block->stmts());
    REQUIRE(then_stmts->size() == 1);

    [[maybe_unused]] const auto ret = as_node<ReturnExpr>(
        as_unwrapped_expr(then_stmts->get(0)));

    const auto nested_expr = as_node<IfExpr>(expr->else_branch());

    const auto int_lit = as_node<IntegerLiteral>(nested_expr->condition());
    REQUIRE(int_lit->value() == 1);

    const auto nested_then_block = as_node<BlockExpr>(
        nested_expr->then_branch());
    const auto nested_then_stmts = as_node<StmtList>(
        nested_then_block->stmts());
    REQUIRE(nested_then_stmts->size() == 1);

    const auto var_x = as_node<VarExpr>(
        as_unwrapped_expr(nested_then_stmts->get(0)));
    REQUIRE(strings.value(var_x->name()) == "x");

    const auto else_block = as_node<BlockExpr>(nested_expr->else_branch());
    const auto else_stmts = as_node<StmtList>(else_block->stmts());
    REQUIRE(else_stmts->size() == 0);
}

TEST_CASE("Parser should recognize while statements", "[parser]") {
    StringTable strings;
    std::string source = "while a == b { c; }";

    auto while_result = parse_statement(source, strings);

    const auto while_stmt = as_node<WhileStmt>(while_result);
    const auto comp = as_binary(
        while_stmt->condition(), BinaryOperator::Equals);

    const auto lhs = as_node<VarExpr>(comp->left());
    REQUIRE(strings.value(lhs->name()) == "a");

    const auto rhs = as_node<VarExpr>(comp->right());
    REQUIRE(strings.value(rhs->name()) == "b");

    const auto block = as_node<BlockExpr>(while_stmt->body());
    const auto stmts = as_node<StmtList>(block->stmts());
    REQUIRE(stmts->size() == 1);

    const auto var = as_node<VarExpr>(as_unwrapped_expr(stmts->get(0)));
    REQUIRE(strings.value(var->name()) == "c");
}

TEST_CASE("Parser should recognize function definitions", "[parser]") {
    StringTable strings;
    std::string source = "func myfunc (a, b) { return; }";

    auto file_result = parse_file(source, strings);

    const auto file = as_node<File>(file_result);
    REQUIRE(file->items()->size() == 1);

    const auto func = as_node<FuncDecl>(file->items()->get(0));
    REQUIRE(strings.value(func->name()) == "myfunc");
    REQUIRE(func->params()->size() == 2);

    const auto param_a = as_node<ParamDecl>(func->params()->get(0));
    REQUIRE(strings.value(param_a->name()) == "a");

    const auto param_b = as_node<ParamDecl>(func->params()->get(1));
    REQUIRE(strings.value(param_b->name()) == "b");

    const auto body = as_node<BlockExpr>(func->body());
    REQUIRE(body->stmts()->size() == 1);

    const auto ret = as_node<ReturnExpr>(
        as_unwrapped_expr(body->stmts()->get(0)));
    REQUIRE(ret->inner() == nullptr);
}

TEST_CASE("Parser should recognize block expressions", "[parser]") {
    StringTable strings;
    std::string source = "var i = { if (a) { } else { } 4; };";

    auto decl_result = parse_statement(source, strings);

    const auto stmt = as_node<DeclStmt>(decl_result);
    const auto sym = as_node<VarDecl>(stmt->decl());
    REQUIRE(strings.value(sym->name()) == "i");

    const auto block = as_node<BlockExpr>(sym->initializer());
    REQUIRE(block->stmts()->size() == 2);

    [[maybe_unused]] const auto if_expr = as_node<IfExpr>(
        as_node<ExprStmt>(block->stmts()->get(0))->expr());

    const auto literal = as_node<IntegerLiteral>(
        as_unwrapped_expr(block->stmts()->get(1)));
    REQUIRE(literal->value() == 4);
}

TEST_CASE("Parser should recognize function calls", "[parser]") {
    StringTable strings;
    std::string source = "f(1)(2, 3)()";

    auto call_result = parse_expression(source, strings);

    const auto call_1 = as_node<CallExpr>(call_result);
    REQUIRE(call_1->args()->size() == 0);

    const auto call_2 = as_node<CallExpr>(call_1->func());
    REQUIRE(call_2->args()->size() == 2);

    const auto two = as_node<IntegerLiteral>(call_2->args()->get(0));
    REQUIRE(two->value() == 2);

    const auto three = as_node<IntegerLiteral>(call_2->args()->get(1));
    REQUIRE(three->value() == 3);

    const auto call_3 = as_node<CallExpr>(call_2->func());
    REQUIRE(call_3->args()->size() == 1);

    const auto one = as_node<IntegerLiteral>(call_3->args()->get(0));
    REQUIRE(one->value() == 1);

    const auto f = as_node<VarExpr>(call_3->func());
    REQUIRE(strings.value(f->name()) == "f");
}

TEST_CASE("Parser should recognize dot expressions", "[parser]") {
    StringTable strings;
    std::string source = "a.b.c";

    auto dot_result = parse_expression(source, strings);

    const auto dot_1 = as_node<DotExpr>(dot_result);
    REQUIRE(strings.value(dot_1->name()) == "c");

    const auto dot_2 = as_node<DotExpr>(dot_1->inner());
    REQUIRE(strings.value(dot_2->name()) == "b");

    const auto var = as_node<VarExpr>(dot_2->inner());
    REQUIRE(strings.value(var->name()) == "a");
}

TEST_CASE("Parser should parse map literals", "[parser]") {
    StringTable strings;
    std::string source = "Map{'a': 3, \"b\": \"test\", 4 + 5: f()}";

    auto map_result = parse_expression(source, strings);
    REQUIRE(map_result);

    const auto lit = as_node<MapLiteral>(map_result);
    REQUIRE(!lit->has_error());
    REQUIRE(lit->entries()->size() == 3);

    const auto entry_a = lit->entries()->get(0);
    const auto lit_a = as_node<StringLiteral>(entry_a->key());
    const auto lit_3 = as_node<IntegerLiteral>(entry_a->value());
    REQUIRE(strings.value(lit_a->value()) == "a");
    REQUIRE(lit_3->value() == 3);

    const auto entry_b = lit->entries()->get(1);
    const auto lit_b = as_node<StringLiteral>(entry_b->key());
    const auto lit_test = as_node<StringLiteral>(entry_b->value());
    REQUIRE(strings.value(lit_b->value()) == "b");
    REQUIRE(strings.value(lit_test->value()) == "test");

    const auto entry_add = lit->entries()->get(2);
    const auto add_op = as_node<BinaryExpr>(entry_add->key());
    const auto fun_call = as_node<CallExpr>(entry_add->value());
    REQUIRE(add_op->operation() == BinaryOperator::Plus);
    REQUIRE(as_node<IntegerLiteral>(add_op->left())->value() == 4);
    REQUIRE(as_node<IntegerLiteral>(add_op->right())->value() == 5);
    REQUIRE(!fun_call->has_error());
}

TEST_CASE("Parser should parse set literals", "[parser]") {
    StringTable strings;
    std::string source = "Set{\"a\", 4, 3+1, f()}";

    auto set_result = parse_expression(source, strings);
    REQUIRE(set_result);

    const auto lit = as_node<SetLiteral>(set_result);
    REQUIRE(!lit->has_error());
    REQUIRE(lit->entries()->size() == 4);

    const auto lit_a = as_node<StringLiteral>(lit->entries()->get(0));
    REQUIRE(strings.value(lit_a->value()) == "a");

    const auto lit_4 = as_node<IntegerLiteral>(lit->entries()->get(1));
    REQUIRE(lit_4->value() == 4);

    const auto op_add = as_node<BinaryExpr>(lit->entries()->get(2));
    REQUIRE(op_add->operation() == BinaryOperator::Plus);
    REQUIRE(as_node<IntegerLiteral>(op_add->left())->value() == 3);
    REQUIRE(as_node<IntegerLiteral>(op_add->right())->value() == 1);

    const auto call = as_node<CallExpr>(lit->entries()->get(3));
    REQUIRE(!call->has_error());
}

TEST_CASE("Parser should parse array literals", "[parser]") {
    StringTable strings;
    std::string source = "[\"a\", 4, 3+1, f()]";

    auto array_result = parse_expression(source, strings);
    REQUIRE(array_result);

    const auto lit = as_node<ArrayLiteral>(array_result);
    REQUIRE(!lit->has_error());
    REQUIRE(lit->entries()->size() == 4);

    const auto lit_a = as_node<StringLiteral>(lit->entries()->get(0));
    REQUIRE(strings.value(lit_a->value()) == "a");

    const auto lit_4 = as_node<IntegerLiteral>(lit->entries()->get(1));
    REQUIRE(lit_4->value() == 4);

    const auto op_add = as_node<BinaryExpr>(lit->entries()->get(2));
    REQUIRE(op_add->operation() == BinaryOperator::Plus);
    REQUIRE(as_node<IntegerLiteral>(op_add->left())->value() == 3);
    REQUIRE(as_node<IntegerLiteral>(op_add->right())->value() == 1);

    const auto call = as_node<CallExpr>(lit->entries()->get(3));
    REQUIRE(!call->has_error());
}

TEST_CASE(
    "Parser should be able to differentiate expressions and tuple literals",
    "[parser]") {
    StringTable strings;

    SECTION("normal parenthesized expression") {
        auto node = parse_expression("(4)", strings);
        auto number = as_node<IntegerLiteral>(node);
        REQUIRE(number->value() == 4);
    }

    SECTION("empty tuple") {
        auto node = parse_expression("()", strings);
        auto tuple = as_node<TupleLiteral>(node);
        auto entries = as_node<ExprList>(tuple->entries());
        REQUIRE(entries->size() == 0);
    }

    SECTION("one element tuple") {
        auto node = parse_expression("(4,)", strings);
        auto tuple = as_node<TupleLiteral>(node);
        auto entries = as_node<ExprList>(tuple->entries());
        REQUIRE(entries->size() == 1);

        auto number = as_node<IntegerLiteral>(entries->get(0));
        REQUIRE(number->value() == 4);
    }

    SECTION("regular tuple") {
        auto node = parse_expression("(\"hello\", #_f)", strings);
        auto tuple = as_node<TupleLiteral>(node);
        auto entries = as_node<ExprList>(tuple->entries());
        REQUIRE(entries->size() == 2);

        auto str = as_node<StringLiteral>(entries->get(0));
        REQUIRE(strings.value(str->value()) == "hello");

        auto sym = as_node<SymbolLiteral>(entries->get(1));
        REQUIRE(strings.value(sym->value()) == "_f");
    }

    SECTION("tuple with trailing comma") {
        auto node = parse_expression("(\"hello\", f, g(3),)", strings);
        auto tuple = as_node<TupleLiteral>(node);
        auto entries = as_node<ExprList>(tuple->entries());
        REQUIRE(entries->size() == 3);

        auto str = as_node<StringLiteral>(entries->get(0));
        REQUIRE(strings.value(str->value()) == "hello");

        auto ident = as_node<VarExpr>(entries->get(1));
        REQUIRE(strings.value(ident->name()) == "f");

        auto call = as_node<CallExpr>(entries->get(2));
        REQUIRE(call->args()->size() == 1);

        auto func_ident = as_node<VarExpr>(call->func());
        REQUIRE(strings.value(func_ident->name()) == "g");

        auto func_arg = as_node<IntegerLiteral>(call->args()->get(0));
        REQUIRE(func_arg->value() == 3);
    }
}

TEST_CASE("Parser supports import statements", "[parser]") {
    StringTable strings;

    SECTION("import path without dots") {
        auto file = parse_file("import foo;", strings);
        REQUIRE(file->items()->size() == 1);

        auto imp = as_node<ImportDecl>(file->items()->get(0));
        REQUIRE(strings.value(imp->name()) == "foo");

        REQUIRE(imp->path_elements().size() == 1);
        REQUIRE(imp->path_elements()[0] == imp->name());
    }

    SECTION("import path with dots") {
        const auto str_foo = strings.insert("foo");
        const auto str_bar = strings.insert("bar");
        const auto str_baz = strings.insert("baz");

        auto file = parse_file("import foo.bar.baz;", strings);
        REQUIRE(file->items()->size() == 1);

        auto imp = as_node<ImportDecl>(file->items()->get(0));
        REQUIRE(strings.value(imp->name()) == "baz");

        REQUIRE(imp->path_elements().size() == 3);
        REQUIRE(imp->path_elements()[0] == str_foo);
        REQUIRE(imp->path_elements()[1] == str_bar);
        REQUIRE(imp->path_elements()[2] == str_baz);
    }
}
