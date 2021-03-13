#include <catch2/catch.hpp>

#include "compiler/ast/casting.hpp"
#include "compiler/ast/decl.hpp"
#include "compiler/ast/expr.hpp"
#include "compiler/ast/stmt.hpp"
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

TEST_CASE("ast should support arrays", "[ast-gen]") {
    struct Test {
        std::string_view source;
        std::vector<int> expected;
    };

    Test tests[] = {
        {"[]", {}},
        {"[1]", {1}},
        {"[1, 2, 3]", {1, 2, 3}},
    };

    for (const auto& test : tests) {
        CAPTURE(test.source);
        CAPTURE(test.expected);

        auto ast = parse_expr_ast(test.source);
        auto array = check<AstArrayLiteral>(ast.root.get());
        auto& items = array->items();
        REQUIRE(items.size() == test.expected.size());

        for (size_t i = 0; i < items.size(); ++i) {
            auto integer = check<AstIntegerLiteral>(items.get(i));
            REQUIRE(integer->value() == test.expected[i]);
        }
    }
}

TEST_CASE("ast should support tuples", "[ast-gen]") {
    struct Test {
        std::string_view source;
        std::vector<int> expected;
    };

    Test tests[] = {
        {"()", {}},
        {"(1,)", {1}},
        {"(1, 2, 3)", {1, 2, 3}},
    };

    for (const auto& test : tests) {
        CAPTURE(test.source);
        CAPTURE(test.expected);

        auto ast = parse_expr_ast(test.source);
        auto tuple = check<AstTupleLiteral>(ast.root.get());
        auto& items = tuple->items();
        REQUIRE(items.size() == test.expected.size());

        for (size_t i = 0; i < items.size(); ++i) {
            auto integer = check<AstIntegerLiteral>(items.get(i));
            REQUIRE(integer->value() == test.expected[i]);
        }
    }
}

TEST_CASE("ast should support simple strings", "[ast-gen]") {
    auto ast = parse_expr_ast("\"hello\"");
    auto string_expr = check<AstStringExpr>(ast.root.get());
    auto& items = string_expr->items();
    REQUIRE(items.size() == 1);

    auto string_literal = check<AstStringLiteral>(items.get(0));
    REQUIRE(ast.strings.value(string_literal->value()) == "hello");
}

TEST_CASE("ast should support strings with escape characters", "[ast-gen]") {
    auto ast = parse_expr_ast(R"("a\nb")");
    auto string_expr = check<AstStringExpr>(ast.root.get());
    auto& items = string_expr->items();
    REQUIRE(items.size() == 1);

    auto string_literal = check<AstStringLiteral>(items.get(0));
    REQUIRE(ast.strings.value(string_literal->value()) == "a\nb");
}

TEST_CASE("ast should support strings with interpolated variables", "[ast-gen]") {
    auto ast = parse_expr_ast("\"hello $name\"");
    auto string_expr = check<AstStringExpr>(ast.root.get());
    auto& items = string_expr->items();
    REQUIRE(items.size() == 2);

    auto string_literal = check<AstStringLiteral>(items.get(0));
    REQUIRE(ast.strings.value(string_literal->value()) == "hello ");

    auto var_expr = check<AstVarExpr>(items.get(1));
    REQUIRE(ast.strings.value(var_expr->name()) == "name");
}

TEST_CASE("ast should support strings with embedded expression blocks", "[ast-gen]") {
    auto ast = parse_expr_ast("\"hello ${1 + 1}!\"");
    auto string_expr = check<AstStringExpr>(ast.root.get());
    auto& items = string_expr->items();
    REQUIRE(items.size() == 3);

    auto hello_literal = check<AstStringLiteral>(items.get(0));
    REQUIRE(ast.strings.value(hello_literal->value()) == "hello ");

    auto binary_expr = check<AstBinaryExpr>(items.get(1));
    REQUIRE(binary_expr->operation() == BinaryOperator::Plus);

    auto excl_literal = check<AstStringLiteral>(items.get(2));
    REQUIRE(ast.strings.value(excl_literal->value()) == "!");
}

TEST_CASE("ast should merge multiple adjacent strings into one expression", "[ast-gen]") {
    auto ast = parse_expr_ast("\"hello\"\"world\"");
    auto string_expr = check<AstStringExpr>(ast.root.get());
    auto& items = string_expr->items();
    REQUIRE(items.size() == 2);

    auto hello_literal = check<AstStringLiteral>(items.get(0));
    REQUIRE(ast.strings.value(hello_literal->value()) == "hello");

    auto world_literal = check<AstStringLiteral>(items.get(1));
    REQUIRE(ast.strings.value(world_literal->value()) == "world");
}

TEST_CASE("ast should support block expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("{ 1; ;; 2; }");
    auto block_expr = check<AstBlockExpr>(ast.root.get());
    auto& stmts = block_expr->stmts();
    REQUIRE(stmts.size() == 2);

    auto lit_1_stmt = check<AstExprStmt>(stmts.get(0));
    auto lit_1 = check<AstIntegerLiteral>(lit_1_stmt->expr());
    REQUIRE(lit_1->value() == 1);

    auto lit_2_stmt = check<AstExprStmt>(stmts.get(1));
    auto lit_2 = check<AstIntegerLiteral>(lit_2_stmt->expr());
    REQUIRE(lit_2->value() == 2);
}

TEST_CASE("ast should support if expressions without an else branch", "[ast-gen]") {
    auto ast = parse_expr_ast("if (1) { 2 + 3; }");
    auto if_expr = check<AstIfExpr>(ast.root.get());

    auto cond = check<AstIntegerLiteral>(if_expr->cond());
    REQUIRE(cond->value() == 1);

    check<AstBlockExpr>(if_expr->then_branch());
    REQUIRE(if_expr->else_branch() == nullptr);
}

TEST_CASE("ast should support if expressions with an else branch", "[ast-gen]") {
    auto ast = parse_expr_ast("if (1) { 2; } else { 3; }");
    auto if_expr = check<AstIfExpr>(ast.root.get());
    auto cond = check<AstIntegerLiteral>(if_expr->cond());
    REQUIRE(cond->value() == 1);

    check<AstBlockExpr>(if_expr->then_branch());
    check<AstBlockExpr>(if_expr->else_branch());
}

TEST_CASE("ast should support function expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("func (a, b) { return a + b; }");
    auto func_expr = check<AstFuncExpr>(ast.root.get());

    auto func_decl = check<AstFuncDecl>(func_expr->decl());
    REQUIRE(!func_decl->name());
    REQUIRE(func_decl->modifiers().empty());
    REQUIRE(!func_decl->body_is_value());

    auto& params = func_decl->params();
    REQUIRE(params.size() == 2);

    auto param_a = params.get(0);
    REQUIRE(ast.strings.value(param_a->name()) == "a");

    auto param_b = params.get(1);
    REQUIRE(ast.strings.value(param_b->name()) == "b");

    auto body = check<AstBlockExpr>(func_decl->body());
    REQUIRE(body->stmts().size() == 1);
}

TEST_CASE("ast should support function expressions with value expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("func () = 42");
    auto func_expr = check<AstFuncExpr>(ast.root.get());

    auto func_decl = check<AstFuncDecl>(func_expr->decl());
    REQUIRE(!func_decl->name());
    REQUIRE(func_decl->modifiers().empty());
    REQUIRE(func_decl->params().empty());
    REQUIRE(func_decl->body_is_value());

    auto body = check<AstIntegerLiteral>(func_decl->body());
    REQUIRE(body->value() == 42);
}

TEST_CASE("ast should support function expressions with a name", "[ast-gen]") {
    auto ast = parse_expr_ast("func foo() = 42");
    auto func_expr = check<AstFuncExpr>(ast.root.get());

    auto func_decl = check<AstFuncDecl>(func_expr->decl());
    REQUIRE(ast.strings.value(func_decl->name()) == "foo");
}

TEST_CASE("ast should support function call expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("foo(1, 2 + 3)");
    auto call_expr = check<AstCallExpr>(ast.root.get());
    REQUIRE(call_expr->access_type() == AccessType::Normal);

    auto func = check<AstVarExpr>(call_expr->func());
    REQUIRE(ast.strings.value(func->name()) == "foo");

    auto& args = call_expr->args();
    REQUIRE(args.size() == 2);

    auto arg_1 = check<AstIntegerLiteral>(args.get(0));
    REQUIRE(arg_1->value() == 1);

    auto arg_2 = check<AstBinaryExpr>(args.get(1));
    REQUIRE(arg_2->operation() == BinaryOperator::Plus);
}

TEST_CASE("ast should support optional function call expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("foo?()");
    auto call_expr = check<AstCallExpr>(ast.root.get());
    REQUIRE(call_expr->access_type() == AccessType::Optional);

    auto func = check<AstVarExpr>(call_expr->func());
    REQUIRE(ast.strings.value(func->name()) == "foo");

    REQUIRE(call_expr->args().empty());
}

TEST_CASE("ast should support set expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("set{1, a}");
    auto set_expr = check<AstSetLiteral>(ast.root.get());

    auto& items = set_expr->items();
    REQUIRE(items.size() == 2);
    check<AstIntegerLiteral>(items.get(0));
    check<AstVarExpr>(items.get(1));
}

TEST_CASE("ast should support map expressions", "[ast-gen]") {
    auto ast = parse_expr_ast("map{1: a, f(): map{}}");
    auto map_expr = check<AstMapLiteral>(ast.root.get());

    auto& items = map_expr->items();
    REQUIRE(items.size() == 2);

    auto item_1 = items.get(0);
    check<AstIntegerLiteral>(item_1->key());
    check<AstVarExpr>(item_1->value());

    auto item_2 = items.get(1);
    check<AstCallExpr>(item_2->key());
    check<AstMapLiteral>(item_2->value());
}

TEST_CASE("ast should support expression statements", "[ast-gen]") {
    auto ast = parse_stmt_ast("f();");
    auto stmt = check<AstExprStmt>(ast.root.get());
    check<AstCallExpr>(stmt->expr());
}

TEST_CASE("ast should support defer statements", "[ast-gen]") {
    auto ast = parse_stmt_ast("defer foo();");
    auto stmt = check<AstDeferStmt>(ast.root.get());
    check<AstCallExpr>(stmt->expr());
}

TEST_CASE("ast should support assert statements", "[ast-gen]") {
    auto ast = parse_stmt_ast("assert (foo);");
    auto stmt = check<AstAssertStmt>(ast.root.get());
    check<AstVarExpr>(stmt->cond());
    REQUIRE(stmt->message() == nullptr);
}

TEST_CASE("ast should support assert statements with a message expression", "[ast-gen]") {
    auto ast = parse_stmt_ast("assert (foo, \"failure\");");
    auto stmt = check<AstAssertStmt>(ast.root.get());
    check<AstVarExpr>(stmt->cond());
    check<AstStringExpr>(stmt->message());
}

TEST_CASE("ast should support simple variable declarations", "[ast-gen]") {
    struct Test {
        std::string source;
        bool expect_const;
    };

    Test tests[] = {
        {"var f = 42;", false},
        {"const f = 42;", true},
    };

    for (const auto& test : tests) {
        CAPTURE(test.source, test.expect_const);

        auto ast = parse_stmt_ast(test.source);
        auto stmt = check<AstDeclStmt>(ast.root.get());

        auto decl = check<AstVarDecl>(stmt->decl());
        REQUIRE(decl->modifiers().empty());

        auto& bindings = decl->bindings();
        REQUIRE(bindings.size() == 1);

        auto binding = check<AstBinding>(bindings.get(0));
        REQUIRE(binding->is_const() == test.expect_const);

        auto spec = check<AstVarBindingSpec>(binding->spec());
        auto name = check<AstStringIdentifier>(spec->name());
        REQUIRE(ast.strings.value(name->value()) == "f");

        auto init = check<AstIntegerLiteral>(binding->init());
        REQUIRE(init->value() == 42);
    }
}

TEST_CASE("ast should support variable declarations without initializer", "[ast-gen]") {
    auto ast = parse_stmt_ast("var x;");
    auto stmt = check<AstDeclStmt>(ast.root.get());

    auto decl = check<AstVarDecl>(stmt->decl());
    REQUIRE(decl->modifiers().empty());

    auto& bindings = decl->bindings();
    REQUIRE(bindings.size() == 1);

    auto binding = check<AstBinding>(bindings.get(0));
    REQUIRE_FALSE(binding->is_const());
    REQUIRE(binding->init() == nullptr);

    auto spec = check<AstVarBindingSpec>(binding->spec());
    auto name = check<AstStringIdentifier>(spec->name());
    REQUIRE(ast.strings.value(name->value()) == "x");
}

TEST_CASE("ast should support multiple variable declarations in a single statement", "[ast-gen]") {
    std::vector<std::string> expected_names = {"x", "y", "z"};

    auto ast = parse_stmt_ast("var x, y, z;");
    auto stmt = check<AstDeclStmt>(ast.root.get());

    auto decl = check<AstVarDecl>(stmt->decl());
    REQUIRE(decl->modifiers().empty());

    auto& bindings = decl->bindings();
    REQUIRE(bindings.size() == expected_names.size());

    for (size_t i = 0; i < expected_names.size(); ++i) {
        const auto& expected = expected_names[i];

        auto binding = check<AstBinding>(bindings.get(i));
        REQUIRE_FALSE(binding->is_const());
        REQUIRE(binding->init() == nullptr);

        auto spec = check<AstVarBindingSpec>(binding->spec());
        auto name = check<AstStringIdentifier>(spec->name());
        REQUIRE(ast.strings.value(name->value()) == expected);
    }
}

TEST_CASE("ast should support variable declarations with tuple patterns", "[ast-gen]") {
    std::vector<std::string> expected_names = {"x", "y", "z"};

    auto ast = parse_stmt_ast("const (x, y, z) = f();");
    auto stmt = check<AstDeclStmt>(ast.root.get());

    auto decl = check<AstVarDecl>(stmt->decl());
    REQUIRE(decl->modifiers().empty());

    auto& bindings = decl->bindings();
    REQUIRE(bindings.size() == 1);

    auto binding = check<AstBinding>(bindings.get(0));
    REQUIRE(binding->is_const());
    check<AstCallExpr>(binding->init());

    auto spec = check<AstTupleBindingSpec>(binding->spec());
    auto& names = spec->names();
    REQUIRE(names.size() == expected_names.size());

    for (size_t i = 0; i < expected_names.size(); ++i) {
        const auto& expected = expected_names[i];

        auto name = check<AstStringIdentifier>(names.get(i));
        REQUIRE(ast.strings.value(name->value()) == expected);
    }
}

TEST_CASE("ast should support while loops", "[ast-gen]") {
    auto ast = parse_stmt_ast("while foo() { std.print(123); }");
    auto stmt = check<AstWhileStmt>(ast.root.get());
    check<AstCallExpr>(stmt->cond());
    check<AstBlockExpr>(stmt->body());
}

TEST_CASE("ast should support old style for loops", "[ast-gen]") {
    auto ast = parse_stmt_ast(R"(
        for var i = 1; i < 5; i += 1 {
            std.print(i);
        }
    )");
    auto stmt = check<AstForStmt>(ast.root.get());
    check<AstVarDecl>(stmt->decl());

    auto cond = check<AstBinaryExpr>(stmt->cond());
    REQUIRE(cond->operation() == BinaryOperator::Less);

    auto step = check<AstBinaryExpr>(stmt->step());
    REQUIRE(step->operation() == BinaryOperator::AssignPlus);

    check<AstBlockExpr>(stmt->body());
}

TEST_CASE("ast should support old style for loops without any header items", "[ast-gen]") {
    auto ast = parse_stmt_ast(R"(
        for ;; {
            std.print(i);
        }
    )");
    auto stmt = check<AstForStmt>(ast.root.get());
    REQUIRE(stmt->decl() == nullptr);
    REQUIRE(stmt->cond() == nullptr);
    REQUIRE(stmt->step() == nullptr);
    check<AstBlockExpr>(stmt->body());
}

TEST_CASE("ast should support for each loops", "[ast-gen]") {
    auto ast = parse_stmt_ast(R"(
        for a in list {
            std.print(a);
        }
    )");
    auto stmt = check<AstForEachStmt>(ast.root.get());
    check<AstVarBindingSpec>(stmt->spec());
    check<AstVarExpr>(stmt->expr());
    check<AstBlockExpr>(stmt->body());
}

TEST_CASE("ast should support import items", "[ast-gen]") {
    auto ast = parse_item_ast("import a.b.c;");
    auto stmt = check<AstDeclStmt>(ast.root.get());

    auto decl = check<AstImportDecl>(stmt->decl());
    REQUIRE(ast.strings.value(decl->name()) == "c");

    auto& path = decl->path();
    REQUIRE(path.size() == 3);
    REQUIRE(ast.strings.value(path[0]) == "a");
    REQUIRE(ast.strings.value(path[1]) == "b");
    REQUIRE(ast.strings.value(path[2]) == "c");
}

TEST_CASE("ast should support var items", "[ast-gen]") {
    auto ast = parse_item_ast("export const x = 123;");
    auto stmt = check<AstDeclStmt>(ast.root.get());

    auto decl = check<AstVarDecl>(stmt->decl());

    auto& modifiers = decl->modifiers();
    REQUIRE(modifiers.size() == 1);
    check<AstExportModifier>(modifiers.get(0));

    auto& bindings = decl->bindings();
    REQUIRE(bindings.size() == 1);

    auto binding = check<AstBinding>(bindings.get(0));
    REQUIRE(binding->is_const());
    check<AstIntegerLiteral>(binding->init());

    auto spec = check<AstVarBindingSpec>(binding->spec());
    auto name = check<AstStringIdentifier>(spec->name());
    REQUIRE(ast.strings.value(name->value()) == "x");
}

TEST_CASE("ast should support function declaration items", "[ast-gen]") {
    auto ast = parse_item_ast("export func foo() {}");
    auto stmt = check<AstDeclStmt>(ast.root.get());

    auto decl = check<AstFuncDecl>(stmt->decl());
    REQUIRE(ast.strings.value(decl->name()) == "foo");
    REQUIRE(decl->params().empty());
    REQUIRE_FALSE(decl->body_is_value());
    check<AstBlockExpr>(decl->body());

    auto& modifiers = decl->modifiers();
    REQUIRE(modifiers.size() == 1);
    check<AstExportModifier>(modifiers.get(0));
}

TEST_CASE("ast should support files", "[ast-gen]") {
    auto ast = parse_file_ast(R"(
        import std;

        ;;;;

        export const foo = 123;
        
        func bar() {}
    )");
    auto file = check<AstFile>(ast.root.get());
    auto& items = file->items();
    REQUIRE(items.size() == 3);

    auto import_item = check<AstDeclStmt>(items.get(0));
    check<AstImportDecl>(import_item->decl());

    auto var_item = check<AstDeclStmt>(items.get(1));
    check<AstVarDecl>(var_item->decl());

    auto func_item = check<AstDeclStmt>(items.get(2));
    check<AstFuncDecl>(func_item->decl());
}
