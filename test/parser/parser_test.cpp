#include <catch.hpp>

#include "tiro/ast/ast.hpp"
#include "tiro/parser/parser.hpp"

#include "../test_parser.hpp"

using namespace tiro;

template<typename T>
static NotNull<T*> as_node(AstNode* node) {
    constexpr auto expected_type = AstNodeTraits<T>::type_id;

    auto result = try_cast<T>(node);
    INFO("Expected node type: " << to_string(expected_type));
    INFO("Got node type: " << (node ? to_string(node->type()) : "null"));
    REQUIRE(result);
    return TIRO_NN(result);
}

static NotNull<AstBinaryExpr*> as_binary(AstNode* node, BinaryOperator op) {
    auto result = as_node<AstBinaryExpr>(node);
    INFO("Expected operation type: " << to_string(op));
    INFO("Got operation type: " << to_string(result->operation()));
    REQUIRE(result->operation() == op);
    return result;
}

static NotNull<AstUnaryExpr*> as_unary(AstNode* node, UnaryOperator op) {
    auto result = as_node<AstUnaryExpr>(node);
    INFO("Expected operation type: " << to_string(op));
    INFO("Got operation type: " << to_string(result->operation()));
    REQUIRE(result->operation() == op);
    return result;
}

static NotNull<AstExpr*> as_unwrapped_expr(AstNode* node) {
    auto result = as_node<AstExprStmt>(node);
    REQUIRE(result->expr());
    return TIRO_NN(result->expr());
}

static NotNull<AstStringLiteral*> as_static_string(AstNode* expr) {
    if (auto lit = try_cast<AstStringLiteral>(expr))
        return TIRO_NN(lit);

    if (auto str = try_cast<AstStringExpr>(expr)) {
        REQUIRE(str->items().size() == 1);
        return as_node<AstStringLiteral>(str->items().get(0));
    }

    FAIL("Not a static string.");
    throw std::logic_error("Unreachable code.");
}

static NotNull<AstIntegerLiteral*> as_integer(AstNode* node, i64 expected) {
    auto lit = as_node<AstIntegerLiteral>(node);
    REQUIRE(lit->value() == expected);
    return lit;
}

static NotNull<AstFloatLiteral*> as_float(AstNode* node, f64 expected) {
    auto lit = as_node<AstFloatLiteral>(node);
    REQUIRE(lit->value() == expected);
    return lit;
}

static NotNull<AstBooleanLiteral*> as_boolean(AstNode* node, bool expected) {
    auto lit = as_node<AstBooleanLiteral>(node);
    REQUIRE(lit->value() == expected);
    return lit;
}

static NotNull<AstCallExpr*>
as_call(AstNode* node, AccessType expected_access_type) {
    auto call = as_node<AstCallExpr>(node);
    CAPTURE(to_string(call->access_type()), to_string(expected_access_type));
    REQUIRE(call->access_type() == expected_access_type);
    return call;
}

static NotNull<AstPropertyExpr*>
as_property(AstNode* node, AccessType expected_access_type) {
    auto prop = as_node<AstPropertyExpr>(node);
    CAPTURE(to_string(prop->access_type()), to_string(expected_access_type));
    REQUIRE(prop->access_type() == expected_access_type);
    return prop;
}

// FIXME: Element syntax (i.e. "a[b]" or "a[b] = c") never tested.
[[maybe_unused]] static NotNull<AstElementExpr*>
as_element(AstNode* node, AccessType expected_access_type) {
    auto elem = as_node<AstElementExpr>(node);
    CAPTURE(to_string(elem->access_type()), to_string(expected_access_type));
    REQUIRE(elem->access_type() == expected_access_type);
    return elem;
}

TEST_CASE("Parser should respect arithmetic operator precendence", "[parser]") {
    std::string_view source = "-4**2 + 1234 * (2.34 - 1)";
    TestParser parser;

    auto expr_result = parser.parse_expr(source);

    auto add = as_binary(expr_result.get(), BinaryOperator::Plus);
    auto exp = as_binary(add->left(), BinaryOperator::Power);

    auto unary_minus = as_unary(exp->left(), UnaryOperator::Minus);
    as_integer(unary_minus->inner(), 4);

    as_integer(exp->right(), 2);

    auto mul = as_binary(add->right(), BinaryOperator::Multiply);
    as_integer(mul->left(), 1234);

    auto inner_sub = as_binary(mul->right(), BinaryOperator::Minus);
    as_float(inner_sub->left(), 2.34);
    as_integer(inner_sub->right(), 1);
}

TEST_CASE(
    "Parser should support operator precedence in assignments", "[parser]") {
    std::string_view source = "a = b = 3 && 4";

    TestParser parser;
    auto expr_result = parser.parse_expr(source);

    auto assign_a = as_binary(expr_result.get(), BinaryOperator::Assign);

    auto var_a = as_node<AstVarExpr>(assign_a->left());
    REQUIRE(parser.value(var_a->name()) == "a");

    auto assign_b = as_binary(assign_a->right(), BinaryOperator::Assign);

    auto var_b = as_node<AstVarExpr>(assign_b->left());
    REQUIRE(parser.value(var_b->name()) == "b");

    auto binop = as_binary(assign_b->right(), BinaryOperator::LogicalAnd);
    as_integer(binop->left(), 3);
    as_integer(binop->right(), 4);
}

TEST_CASE("Parser should recognize binary assignment operators", "[parser]") {
    std::string_view source = "3 + (c = b -= 4 ** 2)";

    TestParser parser;
    auto expr_result = parser.parse_expr(source);

    auto add_expr = as_binary(expr_result.get(), BinaryOperator::Plus);
    as_integer(add_expr->left(), 3);

    auto assign_expr = as_binary(add_expr->right(), BinaryOperator::Assign);

    auto var_c = as_node<AstVarExpr>(assign_expr->left());
    REQUIRE(parser.value(var_c->name()) == "c");

    auto assign_minus_expr = as_binary(
        assign_expr->right(), BinaryOperator::AssignMinus);

    auto var_b = as_node<AstVarExpr>(assign_minus_expr->left());
    REQUIRE(parser.value(var_b->name()) == "b");

    auto pow_expr = as_binary(
        assign_minus_expr->right(), BinaryOperator::Power);
    as_integer(pow_expr->left(), 4);
    as_integer(pow_expr->right(), 2);
}

TEST_CASE("Parser should group successive strings in a list", "[parser]") {
    TestParser parser;

    SECTION("normal string is not grouped") {
        auto node = parser.parse_expr("\"hello world\"");
        auto string = as_static_string(node.get());
        REQUIRE(parser.value(string->value()) == "hello world");
    }

    SECTION("successive strings are grouped") {
        auto node = parser.parse_expr("\"hello\" \" world\"");
        auto group = as_node<AstStringGroupExpr>(node.get());
        auto& list = group->strings();
        REQUIRE(list.size() == 2);

        auto first = as_static_string(list.get(0));
        REQUIRE(parser.value(first->value()) == "hello");

        auto second = as_static_string(list.get(1));
        REQUIRE(parser.value(second->value()) == " world");
    }
}

TEST_CASE("Parser should recognize assert statements", "[parser]") {
    SECTION("form with one argument") {
        std::string_view source = "assert(true);";

        TestParser parser;
        auto stmt_result = parser.parse_stmt(source);

        auto stmt = as_node<AstAssertStmt>(stmt_result.get());
        as_boolean(stmt->cond(), true);
        REQUIRE(stmt->message() == nullptr);
    }

    SECTION("form with two arguements") {
        std::string_view source = "assert(123, \"error message\");";

        TestParser parser;
        auto stmt_result = parser.parse_stmt(source);

        auto stmt = as_node<AstAssertStmt>(stmt_result.get());
        as_integer(stmt->cond(), 123);
        auto str_lit = as_static_string(stmt->message());
        REQUIRE(parser.value(str_lit->value()) == "error message");
    }
}

TEST_CASE("Parser should recognize constant declarations", "[parser]") {
    std::string_view source = "const i = test();";
    TestParser parser;

    auto stmt_result = parser.parse_stmt(source);

    auto stmt = as_node<AstVarStmt>(stmt_result.get());
    auto decl = as_node<AstVarDecl>(stmt->decl());
    auto& bindings = decl->bindings();
    REQUIRE(bindings.size() == 1);

    auto var_binding = as_node<AstVarBinding>(bindings.get(0));
    REQUIRE(parser.value(var_binding->name()) == "i");
    REQUIRE(var_binding->is_const());

    auto init = as_call(var_binding->init(), AccessType::Normal);
    REQUIRE(init->args().size() == 0);

    auto func = as_node<AstVarExpr>(init->func());
    REQUIRE(parser.value(func->name()) == "test");
}

TEST_CASE("Parser should support tuple unpacking declarations", "[parser]") {
    TestParser parser;

    auto stmt_result = parser.parse_stmt("var (a, b, c) = (1, 2, 3);");

    auto stmt = as_node<AstVarStmt>(stmt_result.get());
    auto decl = as_node<AstVarDecl>(stmt->decl());
    auto& bindings = decl->bindings();
    REQUIRE(bindings.size() == 1);

    auto tuple_binding = as_node<AstTupleBinding>(bindings.get(0));
    auto& names = tuple_binding->names();
    REQUIRE(names.size() == 3);
    REQUIRE(parser.value(names.at(0)) == "a");
    REQUIRE(parser.value(names.at(1)) == "b");
    REQUIRE(parser.value(names.at(2)) == "c");
}

TEST_CASE(
    "Parser should support multiple variable bindings in a single statement",
    "[parser]") {
    TestParser parser;

    auto stmt_result = parser.parse_stmt("const a = 4, b = 3, (c, d) = foo();");

    auto stmt = as_node<AstVarStmt>(stmt_result.get());
    auto decl = as_node<AstVarDecl>(stmt->decl());
    auto& bindings = decl->bindings();
    REQUIRE(bindings.size() == 3);

    auto binding_a = as_node<AstVarBinding>(bindings.get(0));
    REQUIRE(binding_a->is_const());
    REQUIRE(parser.value(binding_a->name()) == "a");
    as_integer(binding_a->init(), 4);

    auto binding_b = as_node<AstVarBinding>(bindings.get(1));
    REQUIRE(binding_b->is_const());
    REQUIRE(parser.value(binding_b->name()) == "b");
    as_integer(binding_b->init(), 3);

    auto binding_cd = as_node<AstTupleBinding>(bindings.get(2));
    REQUIRE(binding_cd->is_const());
    auto& binding_cd_names = binding_cd->names();
    REQUIRE(binding_cd_names.size() == 2);
    REQUIRE(parser.value(binding_cd_names.at(0)) == "c");
    REQUIRE(parser.value(binding_cd_names.at(1)) == "d");

    auto init_cd = as_call(binding_cd->init(), AccessType::Normal);
    auto init_cd_var = as_node<AstVarExpr>(init_cd->func());
    REQUIRE(parser.value(init_cd_var->name()) == "foo");
    REQUIRE(init_cd->args().size() == 0);
}

TEST_CASE("Parser should recognize if statements", "[parser]") {
    std::string_view source = "if a { return 3; } else if (1) { x; } else { }";

    TestParser parser;
    auto if_result = parser.parse_stmt(source);

    auto if_expr = as_node<AstIfExpr>(as_unwrapped_expr(if_result.get()));

    auto var_a = as_node<AstVarExpr>(if_expr->cond());
    REQUIRE(parser.value(var_a->name()) == "a");

    auto then_block = as_node<AstBlockExpr>(if_expr->then_branch());
    auto& then_stmts = then_block->stmts();
    REQUIRE(then_stmts.size() == 1);

    auto ret = as_node<AstReturnExpr>(as_unwrapped_expr(then_stmts.get(0)));
    as_integer(ret->value(), 3);

    auto nested_if_expr = as_node<AstIfExpr>(if_expr->else_branch());
    as_integer(nested_if_expr->cond(), 1);

    auto nested_then_block = as_node<AstBlockExpr>(
        nested_if_expr->then_branch());
    auto& nested_then_stmts = nested_then_block->stmts();
    REQUIRE(nested_then_stmts.size() == 1);

    auto var_x = as_node<AstVarExpr>(
        as_unwrapped_expr(nested_then_stmts.get(0)));
    REQUIRE(parser.value(var_x->name()) == "x");

    auto else_block = as_node<AstBlockExpr>(nested_if_expr->else_branch());
    auto& else_stmts = else_block->stmts();
    REQUIRE(else_stmts.size() == 0);
}

TEST_CASE("Parser should recognize while statements", "[parser]") {
    std::string_view source = "while a == b { c; }";

    TestParser parser;
    auto while_result = parser.parse_stmt(source);

    auto while_stmt = as_node<AstWhileStmt>(while_result.get());
    auto comp = as_binary(while_stmt->cond(), BinaryOperator::Equals);

    auto lhs = as_node<AstVarExpr>(comp->left());
    REQUIRE(parser.value(lhs->name()) == "a");

    auto rhs = as_node<AstVarExpr>(comp->right());
    REQUIRE(parser.value(rhs->name()) == "b");

    auto block = as_node<AstBlockExpr>(while_stmt->body());
    auto& stmts = block->stmts();
    REQUIRE(stmts.size() == 1);

    auto var = as_node<AstVarExpr>(as_unwrapped_expr(stmts.get(0)));
    REQUIRE(parser.value(var->name()) == "c");
}

TEST_CASE("Parser should recognize function definitions", "[parser]") {
    std::string_view source = "func myfunc (a, b) { return; }";

    TestParser parser;
    auto file_result = parser.parse_file(source);

    auto file = as_node<AstFile>(file_result.get());
    REQUIRE(file->items().size() == 1);

    auto item = as_node<AstFuncItem>(file->items().get(0));
    auto func = as_node<AstFuncDecl>(item->decl());
    REQUIRE(parser.value(func->name()) == "myfunc");
    REQUIRE(func->params().size() == 2);

    auto param_a = as_node<AstParamDecl>(func->params().get(0));
    REQUIRE(parser.value(param_a->name()) == "a");

    auto param_b = as_node<AstParamDecl>(func->params().get(1));
    REQUIRE(parser.value(param_b->name()) == "b");

    auto body = as_node<AstBlockExpr>(func->body());
    REQUIRE(body->stmts().size() == 1);

    auto ret = as_node<AstReturnExpr>(as_unwrapped_expr(body->stmts().get(0)));
    REQUIRE(ret->value() == nullptr);
}

TEST_CASE("Parser should recognize block expressions", "[parser]") {
    std::string_view source = "var i = { if (a) { } else { } 4; };";

    TestParser parser;
    auto var_result = parser.parse_stmt(source);

    auto stmt = as_node<AstVarStmt>(var_result.get());
    auto decl = as_node<AstVarDecl>(stmt->decl());
    REQUIRE(decl->bindings().size() == 1);

    auto binding = as_node<AstVarBinding>(decl->bindings().get(0));
    REQUIRE(parser.value(binding->name()) == "i");

    auto block = as_node<AstBlockExpr>(binding->init());
    REQUIRE(block->stmts().size() == 2);

    as_node<AstIfExpr>(as_unwrapped_expr(block->stmts().get(0)));
    as_integer(as_unwrapped_expr(block->stmts().get(1)), 4);
}

TEST_CASE("Parser should recognize function calls", "[parser]") {
    std::string_view source = "f(1)(2, 3)()";

    TestParser parser;
    auto call_result = parser.parse_expr(source);

    auto call_1 = as_call(call_result.get(), AccessType::Normal);
    REQUIRE(call_1->args().size() == 0);

    auto call_2 = as_call(call_1->func(), AccessType::Normal);
    REQUIRE(call_2->args().size() == 2);

    as_integer(call_2->args().get(0), 2);
    as_integer(call_2->args().get(1), 3);

    auto call_3 = as_call(call_2->func(), AccessType::Normal);
    REQUIRE(call_3->args().size() == 1);

    as_integer(call_3->args().get(0), 1);

    auto var_f = as_node<AstVarExpr>(call_3->func());
    REQUIRE(parser.value(var_f->name()) == "f");
}

TEST_CASE("Parser should recognize dot expressions", "[parser]") {
    std::string_view source = "a.b.c";

    TestParser parser;
    auto prop_result = parser.parse_expr(source);

    auto prop_1 = as_property(prop_result.get(), AccessType::Normal);
    auto id_1 = as_node<AstStringIdentifier>(prop_1->property());
    REQUIRE(parser.value(id_1->value()) == "c");

    auto prop_2 = as_property(prop_1->instance(), AccessType::Normal);
    auto id_2 = as_node<AstStringIdentifier>(prop_2->property());
    REQUIRE(parser.value(id_2->value()) == "b");

    auto var = as_node<AstVarExpr>(prop_2->instance());
    REQUIRE(parser.value(var->name()) == "a");
}

TEST_CASE("Parser should parse map literals", "[parser]") {
    std::string_view source = "Map{'a': 3, \"b\": \"test\", 4 + 5: f()}";

    TestParser parser;
    auto map_result = parser.parse_expr(source);

    auto lit = as_node<AstMapLiteral>(map_result.get());
    REQUIRE(!lit->has_error());

    auto& items = lit->items();
    REQUIRE(items.size() == 3);

    auto item_a = items.get(0);
    auto lit_a = as_static_string(item_a->key());
    REQUIRE(parser.value(lit_a->value()) == "a");
    as_integer(item_a->value(), 3);

    auto item_b = items.get(1);
    auto lit_b = as_static_string(item_b->key());
    auto lit_test = as_static_string(item_b->value());
    REQUIRE(parser.value(lit_b->value()) == "b");
    REQUIRE(parser.value(lit_test->value()) == "test");

    auto item_add = items.get(2);
    auto add_op = as_binary(item_add->key(), BinaryOperator::Plus);
    as_integer(add_op->left(), 4);
    as_integer(add_op->right(), 5);

    auto fun_call = as_call(item_add->value(), AccessType::Normal);
    REQUIRE(!fun_call->has_error());
}

TEST_CASE("Parser should parse set literals", "[parser]") {
    std::string_view source = "Set{\"a\", 4, 3+1, f()}";

    TestParser parser;
    auto set_result = parser.parse_expr(source);

    auto lit = as_node<AstSetLiteral>(set_result.get());
    REQUIRE(!lit->has_error());

    auto& items = lit->items();
    REQUIRE(items.size() == 4);

    auto lit_a = as_static_string(items.get(0));
    REQUIRE(parser.value(lit_a->value()) == "a");

    as_integer(items.get(1), 4);

    auto op_add = as_binary(items.get(2), BinaryOperator::Plus);
    as_integer(op_add->left(), 3);
    as_integer(op_add->right(), 1);

    auto call = as_call(items.get(3), AccessType::Normal);
    REQUIRE(!call->has_error());
}

TEST_CASE("Parser should parse array literals", "[parser]") {
    std::string_view source = "[\"a\", 4, 3+1, f()]";

    TestParser parser;
    auto array_result = parser.parse_expr(source);

    auto lit = as_node<AstArrayLiteral>(array_result.get());
    REQUIRE(!lit->has_error());

    auto& items = lit->items();
    REQUIRE(items.size() == 4);

    auto lit_a = as_static_string(items.get(0));
    REQUIRE(parser.value(lit_a->value()) == "a");

    as_integer(items.get(1), 4);

    auto op_add = as_binary(items.get(2), BinaryOperator::Plus);
    as_integer(op_add->left(), 3);
    as_integer(op_add->right(), 1);

    auto call = as_call(items.get(3), AccessType::Normal);
    REQUIRE(!call->has_error());
}

TEST_CASE(
    "Parser should be able to differentiate expressions and tuple literals",
    "[parser]") {
    TestParser parser;

    SECTION("normal parenthesized expression") {
        auto node = parser.parse_expr("(4)");
        as_integer(node.get(), 4);
    }

    SECTION("empty tuple") {
        auto node = parser.parse_expr("()");
        auto tuple = as_node<AstTupleLiteral>(node.get());
        REQUIRE(tuple->items().size() == 0);
    }

    SECTION("one element tuple") {
        auto node = parser.parse_expr("(4,)");
        auto tuple = as_node<AstTupleLiteral>(node.get());
        REQUIRE(tuple->items().size() == 1);

        as_integer(tuple->items().get(0), 4);
    }

    SECTION("regular tuple") {
        auto node = parser.parse_expr("(\"hello\", #_f)");
        auto tuple = as_node<AstTupleLiteral>(node.get());

        auto& items = tuple->items();
        REQUIRE(items.size() == 2);

        auto str = as_static_string(items.get(0));
        REQUIRE(parser.value(str->value()) == "hello");

        auto sym = as_node<AstSymbolLiteral>(items.get(1));
        REQUIRE(parser.value(sym->value()) == "_f");
    }

    SECTION("tuple with trailing comma") {
        auto node = parser.parse_expr("(\"hello\", f, g(3),)");
        auto tuple = as_node<AstTupleLiteral>(node.get());

        auto& items = tuple->items();
        REQUIRE(items.size() == 3);

        auto str = as_static_string(items.get(0));
        REQUIRE(parser.value(str->value()) == "hello");

        auto ident = as_node<AstVarExpr>(items.get(1));
        REQUIRE(parser.value(ident->name()) == "f");

        auto call = as_call(items.get(2), AccessType::Normal);
        REQUIRE(call->args().size() == 1);

        auto func_ident = as_node<AstVarExpr>(call->func());
        REQUIRE(parser.value(func_ident->name()) == "g");

        as_integer(call->args().get(0), 3);
    }
}

TEST_CASE("Parser should support tuple member access", "[parser]") {
    TestParser parser;

    auto expr = parser.parse_expr("foo.0 = bar.1.2 = 2");

    auto outer_binop = as_binary(expr.get(), BinaryOperator::Assign);

    auto foo_prop = as_property(outer_binop->left(), AccessType::Normal);
    auto foo_id = as_node<AstNumericIdentifier>(foo_prop->property());
    auto foo_var = as_node<AstVarExpr>(foo_prop->instance());
    REQUIRE(foo_id->value() == 0);
    REQUIRE(parser.value(foo_var->name()) == "foo");

    auto inner_binop = as_binary(outer_binop->right(), BinaryOperator::Assign);

    auto bar_prop_2 = as_property(inner_binop->left(), AccessType::Normal);
    auto bar_id_2 = as_node<AstNumericIdentifier>(bar_prop_2->property());
    REQUIRE(bar_id_2->value() == 2);

    auto bar_prop_1 = as_property(bar_prop_2->instance(), AccessType::Normal);
    auto bar_id_1 = as_node<AstNumericIdentifier>(bar_prop_1->property());
    REQUIRE(bar_id_1->value() == 1);

    auto bar_var = as_node<AstVarExpr>(bar_prop_1->instance());
    REQUIRE(parser.value(bar_var->name()) == "bar");

    as_integer(inner_binop->right(), 2);
}

TEST_CASE("Parser should support tuple unpacking assignment", "[parser]") {
    TestParser parser;

    SECTION("multiple variables") {
        auto expr = parser.parse_expr("(a, b) = foo();");

        auto assign_expr = as_binary(expr.get(), BinaryOperator::Assign);

        auto lhs = as_node<AstTupleLiteral>(assign_expr->left());
        REQUIRE(lhs->items().size() == 2);

        auto var_a = as_node<AstVarExpr>(lhs->items().get(0));
        REQUIRE(parser.value(var_a->name()) == "a");

        auto var_b = as_node<AstVarExpr>(lhs->items().get(1));
        REQUIRE(parser.value(var_b->name()) == "b");
    }

    SECTION("empty tuple") {
        // Valid but useless
        auto expr = parser.parse_expr("() = foo();");

        auto assign_expr = as_binary(expr.get(), BinaryOperator::Assign);
        auto lhs = as_node<AstTupleLiteral>(assign_expr->left());
        REQUIRE(lhs->items().size() == 0);
    }
}

TEST_CASE("Parser should support import statements", "[parser]") {
    TestParser parser;

    SECTION("import path without dots") {
        auto file = parser.parse_file("import foo;");
        REQUIRE(file->items().size() == 1);

        auto item = as_node<AstImportItem>(file->items().get(0));
        REQUIRE(parser.value(item->name()) == "foo");

        REQUIRE(item->path().size() == 1);
        REQUIRE(item->path()[0] == item->name());
    }

    SECTION("import path with dots") {
        auto str_foo = parser.strings().insert("foo");
        auto str_bar = parser.strings().insert("bar");
        auto str_baz = parser.strings().insert("baz");

        auto file = parser.parse_file("import foo.bar.baz;");
        REQUIRE(file->items().size() == 1);

        auto imp = as_node<AstImportItem>(file->items().get(0));
        REQUIRE(parser.value(imp->name()) == "baz");

        REQUIRE(imp->path().size() == 3);
        REQUIRE(imp->path()[0] == str_foo);
        REQUIRE(imp->path()[1] == str_bar);
        REQUIRE(imp->path()[2] == str_baz);
    }
}

TEST_CASE("Parser should support interpolated strings", "[parser]") {
    TestParser parser;

    SECTION("Simple identifier") {
        auto expr_result = parser.parse_expr(R"(
            "hello $world!"
        )");

        auto expr = as_node<AstStringExpr>(expr_result.get());
        auto& items = expr->items();
        REQUIRE(items.size() == 3);

        auto start = as_static_string(items.get(0));
        REQUIRE(parser.value(start->value()) == "hello ");

        auto var = as_node<AstVarExpr>(items.get(1));
        REQUIRE(parser.value(var->name()) == "world");

        auto end = as_static_string(items.get(2));
        REQUIRE(parser.value(end->value()) == "!");
    }

    SECTION("Simple identifier (single quote)") {
        auto expr_result = parser.parse_expr(R"(
            'hello $world!'
        )");

        auto expr = as_node<AstStringExpr>(expr_result.get());
        auto& items = expr->items();
        REQUIRE(items.size() == 3);

        auto start = as_static_string(items.get(0));
        REQUIRE(parser.value(start->value()) == "hello ");

        auto var = as_node<AstVarExpr>(items.get(1));
        REQUIRE(parser.value(var->name()) == "world");

        auto end = as_static_string(items.get(2));
        REQUIRE(parser.value(end->value()) == "!");
    }

    SECTION("Complex expression") {
        auto expr_result = parser.parse_expr(R"RAW(
            "the answer is ${ 21 * 2.0 }"
        )RAW");

        auto expr = as_node<AstStringExpr>(expr_result.get());
        auto& items = expr->items();
        REQUIRE(items.size() == 2);

        auto start = as_static_string(items.get(0));
        REQUIRE(parser.value(start->value()) == "the answer is ");

        auto nested_expr = as_binary(items.get(1), BinaryOperator::Multiply);
        as_integer(nested_expr->left(), 21);
        as_float(nested_expr->right(), 2.0);
    }
}

TEST_CASE(
    "variables and constants should be accepted at module level", "[parser]") {
    TestParser parser;

    SECTION("variable") {
        auto item_result = parser.parse_toplevel_item(R"(
            var foo = a() + 1;
        )");

        auto item = as_node<AstVarItem>(item_result.get());
        auto decl = as_node<AstVarDecl>(item->decl());
        REQUIRE(decl->bindings().size() == 1);

        auto foo_binding = as_node<AstVarBinding>(decl->bindings().get(0));
        REQUIRE(parser.value(foo_binding->name()) == "foo");
        as_binary(foo_binding->init(), BinaryOperator::Plus);
    }

    SECTION("constants") {
        auto item_result = parser.parse_toplevel_item(R"(
            const a = 3, b = (1, 2);
        )");

        auto item = as_node<AstVarItem>(item_result.get());
        auto decl = as_node<AstVarDecl>(item->decl());

        auto& bindings = decl->bindings();
        REQUIRE(bindings.size() == 2);

        auto a_binding = as_node<AstVarBinding>(bindings.get(0));
        REQUIRE(parser.value(a_binding->name()) == "a");

        as_integer(a_binding->init(), 3);

        auto b_binding = as_node<AstVarBinding>(bindings.get(1));
        REQUIRE(parser.value(b_binding->name()) == "b");

        auto b_init = as_node<AstTupleLiteral>(b_binding->init());
        REQUIRE(b_init->items().size() == 2);

        as_integer(b_init->items().get(0), 1);
        as_integer(b_init->items().get(1), 2);
    }

    SECTION("tuple declaration") {
        auto item_result = parser.parse_toplevel_item(R"(
            const (a, b) = (1, 2);
        )");

        auto item = as_node<AstVarItem>(item_result.get());
        auto decl = as_node<AstVarDecl>(item);

        auto& bindings = decl->bindings();
        REQUIRE(bindings.size() == 1);

        auto tuple_binding = as_node<AstTupleBinding>(bindings.get(0));
        auto& names = tuple_binding->names();
        REQUIRE(names.size() == 2);
        REQUIRE(parser.value(names.at(0)) == "a");
        REQUIRE(parser.value(names.at(1)) == "b");

        auto tuple_init = as_node<AstTupleLiteral>(tuple_binding->init());
        REQUIRE(tuple_init->items().size() == 2);
        as_integer(tuple_init->items().get(0), 1);
        as_integer(tuple_init->items().get(1), 2);
    }
}
