#include <catch.hpp>

#include "compiler/ast/ast.hpp"
#include "compiler/parser/parser.hpp"

#include "support/test_parser.hpp"

using namespace tiro;

namespace {

class AstTest final : public TestParser {
public:
    template<typename T>
    NotNull<T*> check_node(AstNode* node) {
        constexpr auto expected_type = AstNodeTraits<T>::type_id;

        auto result = try_cast<T>(node);
        INFO("Expected node type: " << to_string(expected_type));
        INFO("Got node type: " << (node ? to_string(node->type()) : "null"));
        REQUIRE(result);
        return TIRO_NN(result);
    }

    NotNull<AstBinaryExpr*> check_binary(AstNode* node, BinaryOperator op) {
        auto result = check_node<AstBinaryExpr>(node);
        INFO("Expected operation type: " << to_string(op));
        INFO("Got operation type: " << to_string(result->operation()));
        REQUIRE(result->operation() == op);
        return result;
    }

    NotNull<AstUnaryExpr*> check_unary(AstNode* node, UnaryOperator op) {
        auto result = check_node<AstUnaryExpr>(node);
        INFO("Expected operation type: " << to_string(op));
        INFO("Got operation type: " << to_string(result->operation()));
        REQUIRE(result->operation() == op);
        return result;
    }

    NotNull<AstExpr*> check_expr_in_stmt(AstNode* node) {
        auto result = check_node<AstExprStmt>(node);
        REQUIRE(result->expr());
        return TIRO_NN(result->expr());
    }

    NotNull<AstParamDecl*> check_param_decl(AstNode* node, std::string_view expected_name) {
        auto decl = check_node<AstParamDecl>(node);
        REQUIRE(value(decl->name()) == expected_name);
        return decl;
    }

    NotNull<AstBinding*> check_var_binding(AstNode* node, std::string_view expected_name) {
        auto binding = check_node<AstBinding>(node);
        check_var_spec(binding->spec(), expected_name);
        return binding;
    }

    NotNull<AstVarBindingSpec*> check_var_spec(AstNode* node, std::string_view expected_name) {
        auto binding = check_node<AstVarBindingSpec>(node);
        check_string_id(binding->name(), expected_name);
        return binding;
    }

    NotNull<AstStringLiteral*>
    check_static_string(AstNode* expr, std::string_view expected_literal) {
        if (auto lit = try_cast<AstStringLiteral>(expr)) {
            REQUIRE(value(lit->value()) == expected_literal);
            return TIRO_NN(lit);
        }

        if (auto str = try_cast<AstStringExpr>(expr)) {
            REQUIRE(str->items().size() == 1);
            auto lit = check_node<AstStringLiteral>(str->items().get(0));
            REQUIRE(value(lit->value()) == expected_literal);
            return lit;
        }

        FAIL("Not a static string.");
        throw std::logic_error("Unreachable code.");
    }

    NotNull<AstSymbolLiteral*> check_symbol(AstNode* node, std::string_view expected_value) {
        auto lit = check_node<AstSymbolLiteral>(node);
        REQUIRE(value(lit->value()) == expected_value);
        return lit;
    }

    NotNull<AstVarExpr*> check_var_expr(AstNode* node, std::string_view expected_name) {
        auto expr = check_node<AstVarExpr>(node);
        REQUIRE(value(expr->name()) == expected_name);
        return expr;
    }

    NotNull<AstStringIdentifier*> check_string_id(AstNode* node, std::string_view expected_value) {
        auto id = check_node<AstStringIdentifier>(node);
        REQUIRE(value(id->value()) == expected_value);
        return id;
    }

    NotNull<AstNumericIdentifier*> check_numeric_id(AstNode* node, u32 expected_value) {
        auto id = check_node<AstNumericIdentifier>(node);
        REQUIRE(id->value() == expected_value);
        return id;
    }

    NotNull<AstIntegerLiteral*> check_integer(AstNode* node, i64 expected) {
        auto lit = check_node<AstIntegerLiteral>(node);
        REQUIRE(lit->value() == expected);
        return lit;
    }

    NotNull<AstFloatLiteral*> check_float(AstNode* node, f64 expected) {
        auto lit = check_node<AstFloatLiteral>(node);
        REQUIRE(lit->value() == expected);
        return lit;
    }

    NotNull<AstBooleanLiteral*> check_boolean(AstNode* node, bool expected) {
        auto lit = check_node<AstBooleanLiteral>(node);
        REQUIRE(lit->value() == expected);
        return lit;
    }

    NotNull<AstCallExpr*> check_call(AstNode* node, AccessType expected_access_type) {
        auto call = check_node<AstCallExpr>(node);
        CAPTURE(to_string(call->access_type()), to_string(expected_access_type));
        REQUIRE(call->access_type() == expected_access_type);
        return call;
    }

    NotNull<AstPropertyExpr*> check_property(AstNode* node, AccessType expected_access_type) {
        auto prop = check_node<AstPropertyExpr>(node);
        CAPTURE(to_string(prop->access_type()), to_string(expected_access_type));
        REQUIRE(prop->access_type() == expected_access_type);
        return prop;
    }

    // FIXME: Element syntax (i.e. "a[b]" or "a[b] = c") never tested.
    [[maybe_unused]] NotNull<AstElementExpr*>
    check_element(AstNode* node, AccessType expected_access_type) {
        auto elem = check_node<AstElementExpr>(node);
        CAPTURE(to_string(elem->access_type()), to_string(expected_access_type));
        REQUIRE(elem->access_type() == expected_access_type);
        return elem;
    }
};

} // namespace

TEST_CASE("Parser should respect arithmetic operator precendence", "[parser]") {
    std::string_view source = "-4**2 + 1234 * (2.34 - 1)";
    AstTest test;

    auto expr_result = test.parse_expr(source);

    auto add = test.check_binary(expr_result.get(), BinaryOperator::Plus);
    auto exp = test.check_binary(add->left(), BinaryOperator::Power);

    auto unary_minus = test.check_unary(exp->left(), UnaryOperator::Minus);
    test.check_integer(unary_minus->inner(), 4);
    test.check_integer(exp->right(), 2);

    auto mul = test.check_binary(add->right(), BinaryOperator::Multiply);
    test.check_integer(mul->left(), 1234);

    auto inner_sub = test.check_binary(mul->right(), BinaryOperator::Minus);
    test.check_float(inner_sub->left(), 2.34);
    test.check_integer(inner_sub->right(), 1);
}

TEST_CASE("Parser should support operator precedence in assignments", "[parser]") {
    std::string_view source = "a = b = 3 && 4";

    AstTest test;
    auto expr_result = test.parse_expr(source);

    auto assign_a = test.check_binary(expr_result.get(), BinaryOperator::Assign);
    test.check_var_expr(assign_a->left(), "a");

    auto assign_b = test.check_binary(assign_a->right(), BinaryOperator::Assign);
    test.check_var_expr(assign_b->left(), "b");

    auto binop = test.check_binary(assign_b->right(), BinaryOperator::LogicalAnd);
    test.check_integer(binop->left(), 3);
    test.check_integer(binop->right(), 4);
}

TEST_CASE("Parser should recognize binary assignment operators", "[parser]") {
    std::string_view source = "3 + (c = b -= 4 ** 2)";

    AstTest test;
    auto expr_result = test.parse_expr(source);

    auto add_expr = test.check_binary(expr_result.get(), BinaryOperator::Plus);
    test.check_integer(add_expr->left(), 3);

    auto assign_expr = test.check_binary(add_expr->right(), BinaryOperator::Assign);

    test.check_var_expr(assign_expr->left(), "c");

    auto assign_minus_expr = test.check_binary(assign_expr->right(), BinaryOperator::AssignMinus);

    test.check_var_expr(assign_minus_expr->left(), "b");

    auto pow_expr = test.check_binary(assign_minus_expr->right(), BinaryOperator::Power);
    test.check_integer(pow_expr->left(), 4);
    test.check_integer(pow_expr->right(), 2);
}

TEST_CASE("Parser should recognize the null coalescing operator", "[parser]") {
    AstTest test;
    auto expr_result = test.parse_expr("x.y ?? 3");

    auto coalesce_expr = test.check_binary(expr_result.get(), BinaryOperator::NullCoalesce);
    test.check_property(coalesce_expr->left(), AccessType::Normal);
    test.check_integer(coalesce_expr->right(), 3);
}

TEST_CASE("The null coalescing operator has low precedence", "[parser]") {
    AstTest test;
    auto expr_result = test.parse_expr("x ?? 3 - 4");

    auto coalesce_expr = test.check_binary(expr_result.get(), BinaryOperator::NullCoalesce);
    test.check_var_expr(coalesce_expr->left(), "x");

    auto sub_expr = test.check_binary(coalesce_expr->right(), BinaryOperator::Minus);
    test.check_integer(sub_expr->left(), 3);
    test.check_integer(sub_expr->right(), 4);
}

TEST_CASE("Parser should group successive strings in a list", "[parser]") {
    AstTest test;

    SECTION("normal string is not grouped") {
        auto node = test.parse_expr("\"hello world\"");
        test.check_static_string(node.get(), "hello world");
    }

    SECTION("successive strings are grouped") {
        auto node = test.parse_expr("\"hello\" \" world\"");
        auto group = test.check_node<AstStringGroupExpr>(node.get());
        auto& list = group->strings();
        REQUIRE(list.size() == 2);

        test.check_static_string(list.get(0), "hello");
        test.check_static_string(list.get(1), " world");
    }
}

TEST_CASE("Parser should recognize assert statements", "[parser]") {
    SECTION("form with one argument") {
        std::string_view source = "assert(true);";

        AstTest test;
        auto stmt_result = test.parse_stmt(source);

        auto stmt = test.check_node<AstAssertStmt>(stmt_result.get());
        test.check_boolean(stmt->cond(), true);
        REQUIRE(stmt->message() == nullptr);
    }

    SECTION("form with two arguements") {
        std::string_view source = "assert(123, \"error message\");";

        AstTest test;
        auto stmt_result = test.parse_stmt(source);

        auto stmt = test.check_node<AstAssertStmt>(stmt_result.get());
        test.check_integer(stmt->cond(), 123);
        test.check_static_string(stmt->message(), "error message");
    }
}

TEST_CASE("Parser should recognize constant declarations", "[parser]") {
    std::string_view source = "const i = test();";
    AstTest test;

    auto stmt_result = test.parse_stmt(source);

    auto stmt = test.check_node<AstDeclStmt>(stmt_result.get());
    auto decl = test.check_node<AstVarDecl>(stmt->decl());
    auto& bindings = decl->bindings();
    REQUIRE(bindings.size() == 1);

    auto var_binding = test.check_var_binding(bindings.get(0), "i");
    REQUIRE(var_binding->is_const());

    auto init = test.check_call(var_binding->init(), AccessType::Normal);
    REQUIRE(init->args().size() == 0);

    test.check_var_expr(init->func(), "test");
}

TEST_CASE("Parser should support tuple unpacking declarations", "[parser]") {
    AstTest test;

    auto stmt_result = test.parse_stmt("var (a, b, c) = (1, 2, 3);");

    auto stmt = test.check_node<AstDeclStmt>(stmt_result.get());
    auto decl = test.check_node<AstVarDecl>(stmt->decl());
    auto& bindings = decl->bindings();
    REQUIRE(bindings.size() == 1);

    auto binding = test.check_node<AstBinding>(bindings.get(0));
    auto tuple_spec = test.check_node<AstTupleBindingSpec>(binding->spec());
    auto& names = tuple_spec->names();
    REQUIRE(names.size() == 3);
    test.check_string_id(names.get(0), "a");
    test.check_string_id(names.get(1), "b");
    test.check_string_id(names.get(2), "c");
}

TEST_CASE("Parser should support multiple variable bindings in a single statement", "[parser]") {
    AstTest test;

    auto stmt_result = test.parse_stmt("const a = 4, b = 3, (c, d) = foo();");

    auto stmt = test.check_node<AstDeclStmt>(stmt_result.get());
    auto decl = test.check_node<AstVarDecl>(stmt->decl());
    auto& bindings = decl->bindings();
    REQUIRE(bindings.size() == 3);

    auto binding_a = test.check_var_binding(bindings.get(0), "a");
    REQUIRE(binding_a->is_const());
    test.check_integer(binding_a->init(), 4);

    auto binding_b = test.check_var_binding(bindings.get(1), "b");
    REQUIRE(binding_b->is_const());
    test.check_integer(binding_b->init(), 3);

    auto binding_cd = test.check_node<AstBinding>(bindings.get(2));
    REQUIRE(binding_cd->is_const());

    auto binding_cd_spec = test.check_node<AstTupleBindingSpec>(binding_cd->spec());
    auto& binding_cd_names = binding_cd_spec->names();
    REQUIRE(binding_cd_names.size() == 2);
    test.check_string_id(binding_cd_names.get(0), "c");
    test.check_string_id(binding_cd_names.get(1), "d");

    auto init_cd = test.check_call(binding_cd->init(), AccessType::Normal);
    test.check_var_expr(init_cd->func(), "foo");
    REQUIRE(init_cd->args().size() == 0);
}

TEST_CASE("Parser should recognize if statements", "[parser]") {
    std::string_view source = "if a { return 3; } else if (1) { x; } else { }";

    AstTest test;
    auto if_result = test.parse_stmt(source);

    auto if_expr = test.check_node<AstIfExpr>(test.check_expr_in_stmt(if_result.get()));

    test.check_var_expr(if_expr->cond(), "a");

    auto then_block = test.check_node<AstBlockExpr>(if_expr->then_branch());
    auto& then_stmts = then_block->stmts();
    REQUIRE(then_stmts.size() == 1);

    auto ret = test.check_node<AstReturnExpr>(test.check_expr_in_stmt(then_stmts.get(0)));
    test.check_integer(ret->value(), 3);

    auto nested_if_expr = test.check_node<AstIfExpr>(if_expr->else_branch());
    test.check_integer(nested_if_expr->cond(), 1);

    auto nested_then_block = test.check_node<AstBlockExpr>(nested_if_expr->then_branch());
    auto& nested_then_stmts = nested_then_block->stmts();
    REQUIRE(nested_then_stmts.size() == 1);

    test.check_var_expr(test.check_expr_in_stmt(nested_then_stmts.get(0)), "x");

    auto else_block = test.check_node<AstBlockExpr>(nested_if_expr->else_branch());
    auto& else_stmts = else_block->stmts();
    REQUIRE(else_stmts.size() == 0);
}

TEST_CASE("Parser should recognize while statements", "[parser]") {
    std::string_view source = "while a == b { c; }";

    AstTest test;
    auto while_result = test.parse_stmt(source);

    auto while_stmt = test.check_node<AstWhileStmt>(while_result.get());
    auto comp = test.check_binary(while_stmt->cond(), BinaryOperator::Equals);

    test.check_var_expr(comp->left(), "a");
    test.check_var_expr(comp->right(), "b");

    auto block = test.check_node<AstBlockExpr>(while_stmt->body());
    auto& stmts = block->stmts();
    REQUIRE(stmts.size() == 1);

    test.check_var_expr(test.check_expr_in_stmt(stmts.get(0)), "c");
}

TEST_CASE("Parser should recognize function definitions", "[parser]") {
    std::string_view source = "func myfunc (a, b) { return; }";

    AstTest test;
    auto file_result = test.parse_file(source);

    auto file = test.check_node<AstFile>(file_result.get());
    REQUIRE(file->items().size() == 1);

    auto item = test.check_node<AstDeclStmt>(file->items().get(0));
    auto func = test.check_node<AstFuncDecl>(item->decl());
    REQUIRE(test.value(func->name()) == "myfunc");
    REQUIRE(func->params().size() == 2);

    test.check_param_decl(func->params().get(0), "a");
    test.check_param_decl(func->params().get(1), "b");

    auto body = test.check_node<AstBlockExpr>(func->body());
    REQUIRE(body->stmts().size() == 1);

    auto ret = test.check_node<AstReturnExpr>(test.check_expr_in_stmt(body->stmts().get(0)));
    REQUIRE(ret->value() == nullptr);
}

TEST_CASE("Parser should recognize block expressions", "[parser]") {
    std::string_view source = "var i = { if (a) { } else { } 4; };";

    AstTest test;
    auto var_result = test.parse_stmt(source);

    auto stmt = test.check_node<AstDeclStmt>(var_result.get());
    auto decl = test.check_node<AstVarDecl>(stmt->decl());
    REQUIRE(decl->bindings().size() == 1);

    auto binding = test.check_var_binding(decl->bindings().get(0), "i");
    auto block = test.check_node<AstBlockExpr>(binding->init());
    REQUIRE(block->stmts().size() == 2);

    test.check_node<AstIfExpr>(test.check_expr_in_stmt(block->stmts().get(0)));
    test.check_integer(test.check_expr_in_stmt(block->stmts().get(1)), 4);
}

TEST_CASE("Parser should recognize function calls", "[parser]") {
    std::string_view source = "f(1)(2, 3)()";

    AstTest test;
    auto call_result = test.parse_expr(source);

    auto call_1 = test.check_call(call_result.get(), AccessType::Normal);
    REQUIRE(call_1->args().size() == 0);

    auto call_2 = test.check_call(call_1->func(), AccessType::Normal);
    REQUIRE(call_2->args().size() == 2);

    test.check_integer(call_2->args().get(0), 2);
    test.check_integer(call_2->args().get(1), 3);

    auto call_3 = test.check_call(call_2->func(), AccessType::Normal);
    REQUIRE(call_3->args().size() == 1);

    test.check_integer(call_3->args().get(0), 1);

    test.check_var_expr(call_3->func(), "f");
}

TEST_CASE("Parser should recognize property expressions", "[parser]") {
    std::string_view source = "a.b.c";

    AstTest test;
    auto prop_result = test.parse_expr(source);

    auto prop_1 = test.check_property(prop_result.get(), AccessType::Normal);
    test.check_string_id(prop_1->property(), "c");

    auto prop_2 = test.check_property(prop_1->instance(), AccessType::Normal);
    test.check_string_id(prop_2->property(), "b");

    test.check_var_expr(prop_2->instance(), "a");
}

TEST_CASE("Parser should support optional chaining operators", "[parser]") {
    AstTest test;

    SECTION("Property access") {
        auto prop_result = test.parse_expr("a?.b");
        auto prop = test.check_property(prop_result.get(), AccessType::Optional);
        test.check_var_expr(prop->instance(), "a");
        test.check_string_id(prop->property(), "b");
    }

    SECTION("Property access (numeric)") {
        auto prop_result = test.parse_expr("a?.1");
        auto prop = test.check_property(prop_result.get(), AccessType::Optional);
        test.check_var_expr(prop->instance(), "a");
        test.check_numeric_id(prop->property(), 1);
    }

    SECTION("Element access") {
        auto elem_result = test.parse_expr("a?[2]");
        auto elem = test.check_element(elem_result.get(), AccessType::Optional);
        test.check_var_expr(elem->instance(), "a");
        test.check_integer(elem->element(), 2);
    }

    SECTION("Function call") {
        auto call_result = test.parse_expr("a?(0)");
        auto call = test.check_call(call_result.get(), AccessType::Optional);
        test.check_var_expr(call->func(), "a");
        REQUIRE(call->args().size() == 1);
        test.check_integer(call->args().get(0), 0);
    }
}

TEST_CASE("Parser should parse map literals", "[parser]") {
    std::string_view source = "Map{'a': 3, \"b\": \"test\", 4 + 5: f()}";

    AstTest test;
    auto map_result = test.parse_expr(source);

    auto lit = test.check_node<AstMapLiteral>(map_result.get());
    REQUIRE(!lit->has_error());

    auto& items = lit->items();
    REQUIRE(items.size() == 3);

    auto item_a = items.get(0);
    test.check_static_string(item_a->key(), "a");
    test.check_integer(item_a->value(), 3);

    auto item_b = items.get(1);
    test.check_static_string(item_b->key(), "b");
    test.check_static_string(item_b->value(), "test");

    auto item_add = items.get(2);
    auto add_op = test.check_binary(item_add->key(), BinaryOperator::Plus);
    test.check_integer(add_op->left(), 4);
    test.check_integer(add_op->right(), 5);

    auto fun_call = test.check_call(item_add->value(), AccessType::Normal);
    REQUIRE(!fun_call->has_error());
}

TEST_CASE("Parser should parse set literals", "[parser]") {
    std::string_view source = "Set{\"a\", 4, 3+1, f()}";

    AstTest test;
    auto set_result = test.parse_expr(source);

    auto lit = test.check_node<AstSetLiteral>(set_result.get());
    REQUIRE(!lit->has_error());

    auto& items = lit->items();
    REQUIRE(items.size() == 4);

    test.check_static_string(items.get(0), "a");

    test.check_integer(items.get(1), 4);

    auto op_add = test.check_binary(items.get(2), BinaryOperator::Plus);
    test.check_integer(op_add->left(), 3);
    test.check_integer(op_add->right(), 1);

    auto call = test.check_call(items.get(3), AccessType::Normal);
    REQUIRE(!call->has_error());
}

TEST_CASE("Parser should parse array literals", "[parser]") {
    std::string_view source = "[\"a\", 4, 3+1, f()]";

    AstTest test;
    auto array_result = test.parse_expr(source);

    auto lit = test.check_node<AstArrayLiteral>(array_result.get());
    REQUIRE(!lit->has_error());

    auto& items = lit->items();
    REQUIRE(items.size() == 4);

    test.check_static_string(items.get(0), "a");

    test.check_integer(items.get(1), 4);

    auto op_add = test.check_binary(items.get(2), BinaryOperator::Plus);
    test.check_integer(op_add->left(), 3);
    test.check_integer(op_add->right(), 1);

    auto call = test.check_call(items.get(3), AccessType::Normal);
    REQUIRE(!call->has_error());
}

TEST_CASE("Parser should be able to differentiate expressions and tuple literals", "[parser]") {
    AstTest test;

    SECTION("normal parenthesized expression") {
        auto node = test.parse_expr("(4)");
        test.check_integer(node.get(), 4);
    }

    SECTION("empty tuple") {
        auto node = test.parse_expr("()");
        auto tuple = test.check_node<AstTupleLiteral>(node.get());
        REQUIRE(tuple->items().size() == 0);
    }

    SECTION("one element tuple") {
        auto node = test.parse_expr("(4,)");
        auto tuple = test.check_node<AstTupleLiteral>(node.get());
        REQUIRE(tuple->items().size() == 1);

        test.check_integer(tuple->items().get(0), 4);
    }

    SECTION("regular tuple") {
        auto node = test.parse_expr("(\"hello\", #_f)");
        auto tuple = test.check_node<AstTupleLiteral>(node.get());

        auto& items = tuple->items();
        REQUIRE(items.size() == 2);

        test.check_static_string(items.get(0), "hello");
        test.check_symbol(items.get(1), "_f");
    }

    SECTION("tuple with trailing comma") {
        auto node = test.parse_expr("(\"hello\", f, g(3),)");
        auto tuple = test.check_node<AstTupleLiteral>(node.get());

        auto& items = tuple->items();
        REQUIRE(items.size() == 3);

        test.check_static_string(items.get(0), "hello");

        test.check_var_expr(items.get(1), "f");

        auto call = test.check_call(items.get(2), AccessType::Normal);
        REQUIRE(call->args().size() == 1);
        test.check_var_expr(call->func(), "g");
        test.check_integer(call->args().get(0), 3);
    }
}

TEST_CASE("Parser should support tuple member access", "[parser]") {
    AstTest test;

    auto expr = test.parse_expr("foo.0 = bar.1.2 = 2");

    auto outer_binop = test.check_binary(expr.get(), BinaryOperator::Assign);

    auto foo_prop = test.check_property(outer_binop->left(), AccessType::Normal);
    test.check_var_expr(foo_prop->instance(), "foo");
    test.check_numeric_id(foo_prop->property(), 0);

    auto inner_binop = test.check_binary(outer_binop->right(), BinaryOperator::Assign);

    auto bar_prop_2 = test.check_property(inner_binop->left(), AccessType::Normal);
    test.check_numeric_id(bar_prop_2->property(), 2);

    auto bar_prop_1 = test.check_property(bar_prop_2->instance(), AccessType::Normal);
    test.check_numeric_id(bar_prop_1->property(), 1);

    test.check_var_expr(bar_prop_1->instance(), "bar");

    test.check_integer(inner_binop->right(), 2);
}

TEST_CASE("Parser should support tuple unpacking assignment", "[parser]") {
    AstTest test;

    SECTION("multiple variables") {
        auto expr = test.parse_expr("(a, b) = foo();");

        auto assign_expr = test.check_binary(expr.get(), BinaryOperator::Assign);

        auto lhs = test.check_node<AstTupleLiteral>(assign_expr->left());
        REQUIRE(lhs->items().size() == 2);

        test.check_var_expr(lhs->items().get(0), "a");
        test.check_var_expr(lhs->items().get(1), "b");
    }

    SECTION("empty tuple") {
        // Valid but useless
        auto expr = test.parse_expr("() = foo();");

        auto assign_expr = test.check_binary(expr.get(), BinaryOperator::Assign);
        auto lhs = test.check_node<AstTupleLiteral>(assign_expr->left());
        REQUIRE(lhs->items().size() == 0);
    }
}

TEST_CASE("Parser should support import statements", "[parser]") {
    AstTest test;

    SECTION("import path without dots") {
        auto file = test.parse_file("import foo;");
        REQUIRE(file->items().size() == 1);

        auto stmt = test.check_node<AstDeclStmt>(file->items().get(0));
        auto imp = test.check_node<AstImportDecl>(stmt->decl());
        REQUIRE(test.value(imp->name()) == "foo");
        REQUIRE(imp->path().size() == 1);
        REQUIRE(imp->path()[0] == imp->name());
    }

    SECTION("import path with dots") {
        auto str_foo = test.strings().insert("foo");
        auto str_bar = test.strings().insert("bar");
        auto str_baz = test.strings().insert("baz");

        auto file = test.parse_file("import foo.bar.baz;");
        REQUIRE(file->items().size() == 1);

        auto stmt = test.check_node<AstDeclStmt>(file->items().get(0));
        auto imp = test.check_node<AstImportDecl>(stmt->decl());
        REQUIRE(test.value(imp->name()) == "baz");

        REQUIRE(imp->path().size() == 3);
        REQUIRE(imp->path()[0] == str_foo);
        REQUIRE(imp->path()[1] == str_bar);
        REQUIRE(imp->path()[2] == str_baz);
    }
}

TEST_CASE("Parser should support export statements", "[parser]") {
    AstTest test;

    auto file = test.parse_file(R"(
        export import foo;

        export func bar() {
            return 0;
        }

        export const baz = 123;
    )");

    auto require_export_modifier = [&](AstDecl* decl) {
        REQUIRE(decl != nullptr);

        auto& mods = decl->modifiers();
        REQUIRE(mods.size() == 1);

        test.check_node<AstExportModifier>(mods.get(0));
    };

    REQUIRE(file->items().size() == 3);

    auto import_stmt = test.check_node<AstDeclStmt>(file->items().get(0));
    auto import_decl = test.check_node<AstImportDecl>(import_stmt->decl());
    require_export_modifier(import_decl);
    REQUIRE(test.value(import_decl->name()) == "foo");

    auto func_stmt = test.check_node<AstDeclStmt>(file->items().get(1));
    auto func_decl = test.check_node<AstFuncDecl>(func_stmt->decl());
    require_export_modifier(func_decl);
    REQUIRE(test.value(func_decl->name()) == "bar");

    auto var_stmt = test.check_node<AstDeclStmt>(file->items().get(2));
    auto var_decl = test.check_node<AstVarDecl>(var_stmt->decl());
    require_export_modifier(var_decl);
    REQUIRE(var_decl->bindings().size() == 1);
    test.check_var_binding(var_decl->bindings().get(0), "baz");
}

TEST_CASE("Parser should support interpolated strings", "[parser]") {
    AstTest test;

    SECTION("Simple identifier") {
        auto expr_result = test.parse_expr(R"(
            "hello $world!"
        )");

        auto expr = test.check_node<AstStringExpr>(expr_result.get());
        auto& items = expr->items();
        REQUIRE(items.size() == 3);

        test.check_static_string(items.get(0), "hello ");
        test.check_var_expr(items.get(1), "world");
        test.check_static_string(items.get(2), "!");
    }

    SECTION("Simple identifier (single quote)") {
        auto expr_result = test.parse_expr(R"(
            'hello $world!'
        )");

        auto expr = test.check_node<AstStringExpr>(expr_result.get());
        auto& items = expr->items();
        REQUIRE(items.size() == 3);

        test.check_static_string(items.get(0), "hello ");
        test.check_var_expr(items.get(1), "world");
        test.check_static_string(items.get(2), "!");
    }

    SECTION("Complex expression") {
        auto expr_result = test.parse_expr(R"RAW(
            "the answer is ${ 21 * 2.0 }"
        )RAW");

        auto expr = test.check_node<AstStringExpr>(expr_result.get());
        auto& items = expr->items();
        REQUIRE(items.size() == 2);

        test.check_static_string(items.get(0), "the answer is ");

        auto nested_expr = test.check_binary(items.get(1), BinaryOperator::Multiply);
        test.check_integer(nested_expr->left(), 21);
        test.check_float(nested_expr->right(), 2.0);
    }
}

TEST_CASE("Variables and constants should be accepted at module level", "[parser]") {
    AstTest test;

    SECTION("variable") {
        auto stmt_result = test.parse_toplevel_item(R"(
            var foo = a() + 1;
        )");

        auto stmt = test.check_node<AstDeclStmt>(stmt_result.get());
        auto decl = test.check_node<AstVarDecl>(stmt->decl());
        REQUIRE(decl->bindings().size() == 1);

        auto foo_binding = test.check_var_binding(decl->bindings().get(0), "foo");
        test.check_binary(foo_binding->init(), BinaryOperator::Plus);
    }

    SECTION("constants") {
        auto stmt_result = test.parse_toplevel_item(R"(
            const a = 3, b = (1, 2);
        )");

        auto item = test.check_node<AstDeclStmt>(stmt_result.get());
        auto decl = test.check_node<AstVarDecl>(item->decl());

        auto& bindings = decl->bindings();
        REQUIRE(bindings.size() == 2);

        auto a_binding = test.check_var_binding(bindings.get(0), "a");
        test.check_integer(a_binding->init(), 3);

        auto b_binding = test.check_var_binding(bindings.get(1), "b");
        auto b_init = test.check_node<AstTupleLiteral>(b_binding->init());
        REQUIRE(b_init->items().size() == 2);

        test.check_integer(b_init->items().get(0), 1);
        test.check_integer(b_init->items().get(1), 2);
    }

    SECTION("tuple declaration") {
        auto stmt_result = test.parse_toplevel_item(R"(
            const (a, b) = (1, 2);
        )");

        auto item = test.check_node<AstDeclStmt>(stmt_result.get());
        auto decl = test.check_node<AstVarDecl>(item->decl());

        auto& bindings = decl->bindings();
        REQUIRE(bindings.size() == 1);

        auto binding = test.check_node<AstBinding>(bindings.get(0));
        auto tuple_spec = test.check_node<AstTupleBindingSpec>(binding->spec());
        auto& names = tuple_spec->names();
        REQUIRE(names.size() == 2);
        test.check_string_id(names.get(0), "a");
        test.check_string_id(names.get(1), "b");

        auto tuple_init = test.check_node<AstTupleLiteral>(binding->init());
        REQUIRE(tuple_init->items().size() == 2);
        test.check_integer(tuple_init->items().get(0), 1);
        test.check_integer(tuple_init->items().get(1), 2);
    }
}

TEST_CASE("The parser should recognize defer statements", "[parser]") {
    AstTest test;

    auto stmt_result = test.parse_stmt("defer cleanup(foo);");

    auto stmt = test.check_node<AstDeferStmt>(stmt_result.get());
    auto call = test.check_call(stmt->expr(), AccessType::Normal);
    test.check_var_expr(call->func(), "cleanup");

    REQUIRE(call->args().size() == 1);
    test.check_var_expr(call->args().get(0), "foo");
}
