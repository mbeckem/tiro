#include <catch.hpp>

#include "hammer/compiler/syntax/ast.hpp"
#include "hammer/compiler/syntax/parser.hpp"

#include "../test_parser.hpp"

using namespace hammer;
using namespace hammer::compiler;

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
    std::string_view source = "-4**2 + 1234 * (2.34 - 1)";
    TestParser parser;

    auto expr_result = parser.parse_expr(source);

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
    std::string_view source = "a = b = 3 && 4";

    TestParser parser;
    auto expr_result = parser.parse_expr(source);

    const auto assign_a = as_binary(expr_result, BinaryOperator::Assign);

    const auto var_a = as_node<VarExpr>(assign_a->left());
    REQUIRE(parser.value(var_a->name()) == "a");

    const auto assign_b = as_binary(assign_a->right(), BinaryOperator::Assign);

    const auto var_b = as_node<VarExpr>(assign_b->left());
    REQUIRE(parser.value(var_b->name()) == "b");

    const auto binop = as_binary(assign_b->right(), BinaryOperator::LogicalAnd);

    const auto lit_3 = as_node<IntegerLiteral>(binop->left());
    REQUIRE(lit_3->value() == 3);

    const auto lit_4 = as_node<IntegerLiteral>(binop->right());
    REQUIRE(lit_4->value() == 4);
}

TEST_CASE("Parser should group successive strings in a list", "[parser]") {
    TestParser parser;
    SECTION("normal string is not grouped") {
        auto node = parser.parse_expr("\"hello world\"");
        auto string = as_node<StringLiteral>(node);
        REQUIRE(parser.value(string->value()) == "hello world");
    }

    SECTION("successive strings are grouped") {
        auto node = parser.parse_expr("\"hello\" \" world\"");
        auto sequence = as_node<StringSequenceExpr>(node);
        auto list = as_node<ExprList>(sequence->strings());
        REQUIRE(list->size() == 2);

        auto first = as_node<StringLiteral>(list->get(0));
        REQUIRE(parser.value(first->value()) == "hello");

        auto second = as_node<StringLiteral>(list->get(1));
        REQUIRE(parser.value(second->value()) == " world");
    }
}

TEST_CASE("Parser should recognize assert statements", "[parser]") {
    SECTION("form with one argument") {
        std::string_view source = "assert(true);";

        TestParser parser;
        auto stmt_result = parser.parse_stmt(source);

        const auto stmt = as_node<AssertStmt>(stmt_result);
        const auto true_lit = as_node<BooleanLiteral>(stmt->condition());
        REQUIRE(true_lit->value() == true);
        REQUIRE(stmt->message() == nullptr);
    }

    SECTION("form with two arguements") {
        std::string_view source = "assert(123, \"error message\");";

        TestParser parser;
        auto stmt_result = parser.parse_stmt(source);

        const auto stmt = as_node<AssertStmt>(stmt_result);

        const auto int_lit = as_node<IntegerLiteral>(stmt->condition());
        REQUIRE(int_lit->value() == 123);

        const auto str_lit = as_node<StringLiteral>(stmt->message());
        REQUIRE(parser.value(str_lit->value()) == "error message");
    }
}

TEST_CASE("Parser should recognize constant declarations", "[parser]") {
    std::string_view source = "const i = test();";
    TestParser parser;

    auto decl_result = parser.parse_stmt(source);

    const auto stmt = as_node<DeclStmt>(decl_result);
    const auto bindings = as_node<BindingList>(stmt->bindings());
    REQUIRE(bindings->size() == 1);

    const auto var_binding = as_node<VarBinding>(bindings->get(0));
    const auto i_sym = as_node<VarDecl>(var_binding->var());
    REQUIRE(parser.value(i_sym->name()) == "i");
    REQUIRE(i_sym->is_const());

    const auto init = as_node<CallExpr>(var_binding->init());
    const auto args = as_node<ExprList>(init->args());
    REQUIRE(args->size() == 0);

    const auto func = as_node<VarExpr>(init->func());
    REQUIRE(parser.value(func->name()) == "test");
}

TEST_CASE("Parser should support tuple unpacking declarations", "[parser]") {
    TestParser parser;

    const auto result = parser.parse_stmt("var (a, b, c) = (1, 2, 3);");

    const auto stmt = as_node<DeclStmt>(result);
    const auto bindings = as_node<BindingList>(stmt->bindings());
    REQUIRE(bindings->size() == 1);

    const auto tuple_binding = as_node<TupleBinding>(bindings->get(0));
    const auto vars = as_node<VarList>(tuple_binding->vars());
    REQUIRE(vars->size() == 3);

    const auto var_a = as_node<VarDecl>(vars->get(0));
    REQUIRE(parser.value(var_a->name()) == "a");

    const auto var_b = as_node<VarDecl>(vars->get(1));
    REQUIRE(parser.value(var_b->name()) == "b");

    const auto var_c = as_node<VarDecl>(vars->get(2));
    REQUIRE(parser.value(var_c->name()) == "c");
}

TEST_CASE(
    "Parser should support multiple variable bindings in a single statement",
    "[parser]") {
    TestParser parser;

    const auto result = parser.parse_stmt(
        "const a = 4, b = 3, (c, d) = foo();");

    const auto stmt = as_node<DeclStmt>(result);
    const auto bindings = as_node<BindingList>(stmt->bindings());
    REQUIRE(bindings->size() == 3);

    const auto binding_a = as_node<VarBinding>(bindings->get(0));
    const auto var_a = as_node<VarDecl>(binding_a->var());
    const auto init_a = as_node<IntegerLiteral>(binding_a->init());
    REQUIRE(parser.value(var_a->name()) == "a");
    REQUIRE(var_a->is_const() == true);
    REQUIRE(init_a->value() == 4);

    const auto binding_b = as_node<VarBinding>(bindings->get(1));
    const auto var_b = as_node<VarDecl>(binding_b->var());
    const auto init_b = as_node<IntegerLiteral>(binding_b->init());
    REQUIRE(parser.value(var_b->name()) == "b");
    REQUIRE(var_b->is_const() == true);
    REQUIRE(init_b->value() == 3);

    const auto binding_cd = as_node<TupleBinding>(bindings->get(2));
    const auto binding_cd_vars = binding_cd->vars();
    REQUIRE(binding_cd_vars->size() == 2);

    const auto var_c = binding_cd_vars->get(0);
    REQUIRE(parser.value(var_c->name()) == "c");
    REQUIRE(var_c->is_const());

    const auto var_d = binding_cd_vars->get(1);
    REQUIRE(parser.value(var_d->name()) == "d");
    REQUIRE(var_d->is_const());

    const auto init_cd = as_node<CallExpr>(binding_cd->init());
    REQUIRE(init_cd->args()->size() == 0);

    const auto init_cd_call = as_node<VarExpr>(init_cd->func());
    REQUIRE(parser.value(init_cd_call->name()) == "foo");
}

TEST_CASE("Parser should recognize if statements", "[parser]") {
    std::string_view source = "if a { return 3; } else if (1) { x; } else { }";

    TestParser parser;
    auto if_result = parser.parse_stmt(source);

    const auto expr = as_node<IfExpr>(as_node<ExprStmt>(if_result)->expr());

    const auto var_a = as_node<VarExpr>(expr->condition());
    REQUIRE(parser.value(var_a->name()) == "a");

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
    REQUIRE(parser.value(var_x->name()) == "x");

    const auto else_block = as_node<BlockExpr>(nested_expr->else_branch());
    const auto else_stmts = as_node<StmtList>(else_block->stmts());
    REQUIRE(else_stmts->size() == 0);
}

TEST_CASE("Parser should recognize while statements", "[parser]") {
    std::string_view source = "while a == b { c; }";

    TestParser parser;
    auto while_result = parser.parse_stmt(source);

    const auto while_stmt = as_node<WhileStmt>(while_result);
    const auto comp = as_binary(
        while_stmt->condition(), BinaryOperator::Equals);

    const auto lhs = as_node<VarExpr>(comp->left());
    REQUIRE(parser.value(lhs->name()) == "a");

    const auto rhs = as_node<VarExpr>(comp->right());
    REQUIRE(parser.value(rhs->name()) == "b");

    const auto block = as_node<BlockExpr>(while_stmt->body());
    const auto stmts = as_node<StmtList>(block->stmts());
    REQUIRE(stmts->size() == 1);

    const auto var = as_node<VarExpr>(as_unwrapped_expr(stmts->get(0)));
    REQUIRE(parser.value(var->name()) == "c");
}

TEST_CASE("Parser should recognize function definitions", "[parser]") {
    std::string_view source = "func myfunc (a, b) { return; }";

    TestParser parser;
    auto file_result = parser.parse_file(source);

    const auto file = as_node<File>(file_result);
    REQUIRE(file->items()->size() == 1);

    const auto func = as_node<FuncDecl>(file->items()->get(0));
    REQUIRE(parser.value(func->name()) == "myfunc");
    REQUIRE(func->params()->size() == 2);

    const auto param_a = as_node<ParamDecl>(func->params()->get(0));
    REQUIRE(parser.value(param_a->name()) == "a");

    const auto param_b = as_node<ParamDecl>(func->params()->get(1));
    REQUIRE(parser.value(param_b->name()) == "b");

    const auto body = as_node<BlockExpr>(func->body());
    REQUIRE(body->stmts()->size() == 1);

    const auto ret = as_node<ReturnExpr>(
        as_unwrapped_expr(body->stmts()->get(0)));
    REQUIRE(ret->inner() == nullptr);
}

TEST_CASE("Parser should recognize block expressions", "[parser]") {
    std::string_view source = "var i = { if (a) { } else { } 4; };";

    TestParser parser;
    auto decl_result = parser.parse_stmt(source);

    const auto stmt = as_node<DeclStmt>(decl_result);
    REQUIRE(stmt->bindings()->size() == 1);

    const auto binding = as_node<VarBinding>(stmt->bindings()->get(0));
    const auto sym = as_node<VarDecl>(binding->var());
    REQUIRE(parser.value(sym->name()) == "i");

    const auto block = as_node<BlockExpr>(binding->init());
    REQUIRE(block->stmts()->size() == 2);

    [[maybe_unused]] const auto if_expr = as_node<IfExpr>(
        as_node<ExprStmt>(block->stmts()->get(0))->expr());

    const auto literal = as_node<IntegerLiteral>(
        as_unwrapped_expr(block->stmts()->get(1)));
    REQUIRE(literal->value() == 4);
}

TEST_CASE("Parser should recognize function calls", "[parser]") {
    std::string_view source = "f(1)(2, 3)()";

    TestParser parser;
    auto call_result = parser.parse_expr(source);

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
    REQUIRE(parser.value(f->name()) == "f");
}

TEST_CASE("Parser should recognize dot expressions", "[parser]") {
    std::string_view source = "a.b.c";

    TestParser parser;
    auto dot_result = parser.parse_expr(source);

    const auto dot_1 = as_node<DotExpr>(dot_result);
    REQUIRE(parser.value(dot_1->name()) == "c");

    const auto dot_2 = as_node<DotExpr>(dot_1->inner());
    REQUIRE(parser.value(dot_2->name()) == "b");

    const auto var = as_node<VarExpr>(dot_2->inner());
    REQUIRE(parser.value(var->name()) == "a");
}

TEST_CASE("Parser should parse map literals", "[parser]") {
    std::string_view source = "Map{'a': 3, \"b\": \"test\", 4 + 5: f()}";

    TestParser parser;
    auto map_result = parser.parse_expr(source);

    const auto lit = as_node<MapLiteral>(map_result);
    REQUIRE(!lit->has_error());
    REQUIRE(lit->entries()->size() == 3);

    const auto entry_a = lit->entries()->get(0);
    const auto lit_a = as_node<StringLiteral>(entry_a->key());
    const auto lit_3 = as_node<IntegerLiteral>(entry_a->value());
    REQUIRE(parser.value(lit_a->value()) == "a");
    REQUIRE(lit_3->value() == 3);

    const auto entry_b = lit->entries()->get(1);
    const auto lit_b = as_node<StringLiteral>(entry_b->key());
    const auto lit_test = as_node<StringLiteral>(entry_b->value());
    REQUIRE(parser.value(lit_b->value()) == "b");
    REQUIRE(parser.value(lit_test->value()) == "test");

    const auto entry_add = lit->entries()->get(2);
    const auto add_op = as_node<BinaryExpr>(entry_add->key());
    const auto fun_call = as_node<CallExpr>(entry_add->value());
    REQUIRE(add_op->operation() == BinaryOperator::Plus);
    REQUIRE(as_node<IntegerLiteral>(add_op->left())->value() == 4);
    REQUIRE(as_node<IntegerLiteral>(add_op->right())->value() == 5);
    REQUIRE(!fun_call->has_error());
}

TEST_CASE("Parser should parse set literals", "[parser]") {
    std::string_view source = "Set{\"a\", 4, 3+1, f()}";

    TestParser parser;
    auto set_result = parser.parse_expr(source);

    const auto lit = as_node<SetLiteral>(set_result);
    REQUIRE(!lit->has_error());
    REQUIRE(lit->entries()->size() == 4);

    const auto lit_a = as_node<StringLiteral>(lit->entries()->get(0));
    REQUIRE(parser.value(lit_a->value()) == "a");

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
    std::string_view source = "[\"a\", 4, 3+1, f()]";

    TestParser parser;
    auto array_result = parser.parse_expr(source);

    const auto lit = as_node<ArrayLiteral>(array_result);
    REQUIRE(!lit->has_error());
    REQUIRE(lit->entries()->size() == 4);

    const auto lit_a = as_node<StringLiteral>(lit->entries()->get(0));
    REQUIRE(parser.value(lit_a->value()) == "a");

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
    TestParser parser;

    SECTION("normal parenthesized expression") {
        auto node = parser.parse_expr("(4)");
        auto number = as_node<IntegerLiteral>(node);
        REQUIRE(number->value() == 4);
    }

    SECTION("empty tuple") {
        auto node = parser.parse_expr("()");
        auto tuple = as_node<TupleLiteral>(node);
        auto entries = as_node<ExprList>(tuple->entries());
        REQUIRE(entries->size() == 0);
    }

    SECTION("one element tuple") {
        auto node = parser.parse_expr("(4,)");
        auto tuple = as_node<TupleLiteral>(node);
        auto entries = as_node<ExprList>(tuple->entries());
        REQUIRE(entries->size() == 1);

        auto number = as_node<IntegerLiteral>(entries->get(0));
        REQUIRE(number->value() == 4);
    }

    SECTION("regular tuple") {
        auto node = parser.parse_expr("(\"hello\", #_f)");
        auto tuple = as_node<TupleLiteral>(node);
        auto entries = as_node<ExprList>(tuple->entries());
        REQUIRE(entries->size() == 2);

        auto str = as_node<StringLiteral>(entries->get(0));
        REQUIRE(parser.value(str->value()) == "hello");

        auto sym = as_node<SymbolLiteral>(entries->get(1));
        REQUIRE(parser.value(sym->value()) == "_f");
    }

    SECTION("tuple with trailing comma") {
        auto node = parser.parse_expr("(\"hello\", f, g(3),)");
        auto tuple = as_node<TupleLiteral>(node);
        auto entries = as_node<ExprList>(tuple->entries());
        REQUIRE(entries->size() == 3);

        auto str = as_node<StringLiteral>(entries->get(0));
        REQUIRE(parser.value(str->value()) == "hello");

        auto ident = as_node<VarExpr>(entries->get(1));
        REQUIRE(parser.value(ident->name()) == "f");

        auto call = as_node<CallExpr>(entries->get(2));
        REQUIRE(call->args()->size() == 1);

        auto func_ident = as_node<VarExpr>(call->func());
        REQUIRE(parser.value(func_ident->name()) == "g");

        auto func_arg = as_node<IntegerLiteral>(call->args()->get(0));
        REQUIRE(func_arg->value() == 3);
    }
}

TEST_CASE("Parser should support tuple member access", "[parser]") {
    TestParser parser;

    auto expr = parser.parse_expr("foo.0 = bar.1.2 = 2");

    auto outer_binop = as_node<BinaryExpr>(expr);

    auto foo_access = as_node<TupleMemberExpr>(outer_binop->left());
    auto foo_var = as_node<VarExpr>(foo_access->inner());
    REQUIRE(foo_access->index() == 0);

    auto inner_binop = as_node<BinaryExpr>(outer_binop->right());

    auto bar_access_2 = as_node<TupleMemberExpr>(inner_binop->left());
    auto bar_access_1 = as_node<TupleMemberExpr>(bar_access_2->inner());
    auto bar_var = as_node<VarExpr>(bar_access_1->inner());
    REQUIRE(bar_access_2->index() == 2);
    REQUIRE(bar_access_1->index() == 1);

    auto lit_2 = as_node<IntegerLiteral>(inner_binop->right());
    REQUIRE(lit_2->value() == 2);
}

TEST_CASE("Parser should support tuple unpacking assignment", "[parser]") {
    TestParser parser;

    SECTION("multiple variables") {
        const auto expr = parser.parse_expr("(a, b) = foo();");

        const auto assign_expr = as_node<BinaryExpr>(expr);
        REQUIRE(assign_expr->operation() == BinaryOperator::Assign);

        const auto lhs = as_node<TupleLiteral>(assign_expr->left());
        REQUIRE(lhs->entries()->size() == 2);

        const auto var_a = as_node<VarExpr>(lhs->entries()->get(0));
        REQUIRE(parser.value(var_a->name()) == "a");

        const auto var_b = as_node<VarExpr>(lhs->entries()->get(1));
        REQUIRE(parser.value(var_b->name()) == "b");
    }

    SECTION("empty tuple") {
        // Valid but useless
        const auto expr = parser.parse_expr("() = foo();");

        const auto assign_expr = as_node<BinaryExpr>(expr);
        REQUIRE(assign_expr->operation() == BinaryOperator::Assign);

        const auto lhs = as_node<TupleLiteral>(assign_expr->left());
        REQUIRE(lhs->entries()->size() == 0);
    }
}

TEST_CASE("Parser should support import statements", "[parser]") {
    TestParser parser;

    SECTION("import path without dots") {
        auto file = parser.parse_file("import foo;");
        REQUIRE(file->items()->size() == 1);

        auto imp = as_node<ImportDecl>(file->items()->get(0));
        REQUIRE(parser.value(imp->name()) == "foo");

        REQUIRE(imp->path_elements().size() == 1);
        REQUIRE(imp->path_elements()[0] == imp->name());
    }

    SECTION("import path with dots") {
        const auto str_foo = parser.strings().insert("foo");
        const auto str_bar = parser.strings().insert("bar");
        const auto str_baz = parser.strings().insert("baz");

        auto file = parser.parse_file("import foo.bar.baz;");
        REQUIRE(file->items()->size() == 1);

        auto imp = as_node<ImportDecl>(file->items()->get(0));
        REQUIRE(parser.value(imp->name()) == "baz");

        REQUIRE(imp->path_elements().size() == 3);
        REQUIRE(imp->path_elements()[0] == str_foo);
        REQUIRE(imp->path_elements()[1] == str_bar);
        REQUIRE(imp->path_elements()[2] == str_baz);
    }
}
