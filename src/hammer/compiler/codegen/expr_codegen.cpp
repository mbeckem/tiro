#include "hammer/compiler/codegen/expr_codegen.hpp"

#include "hammer/ast/expr.hpp"
#include "hammer/ast/node_visit.hpp"
#include "hammer/core/overloaded.hpp"

namespace hammer {

ExprCodegen::ExprCodegen(ast::Expr& expr, FunctionCodegen& func)
    : expr_(expr)
    , func_(func)
    , builder_(func.builder())
    , strings_(func.strings())
    , diag_(func.diag()) {}

void ExprCodegen::generate() {
    HAMMER_ASSERT(!expr_.has_error(), "Invalid expression node.");

    ast::visit(expr_, [&](auto&& e) { gen_impl(e); });
}

void ExprCodegen::gen_impl(ast::UnaryExpr& e) {
    switch (e.operation()) {
#define HAMMER_SIMPLE_UNARY(op, opcode)       \
    case ast::UnaryOperator::op:              \
        func_.generate_expr_value(e.inner()); \
        builder_.opcode();                    \
        break;

        HAMMER_SIMPLE_UNARY(Plus, upos)
        HAMMER_SIMPLE_UNARY(Minus, uneg);
        HAMMER_SIMPLE_UNARY(BitwiseNot, bnot);
        HAMMER_SIMPLE_UNARY(LogicalNot, lnot);
#undef HAMMER_SIMPLE_UNARY
    }
}

void ExprCodegen::gen_impl(ast::BinaryExpr& e) {
    switch (e.operation()) {
    case ast::BinaryOperator::Assign:
        gen_assign(&e);
        break;

    case ast::BinaryOperator::LogicalAnd:
        gen_logical_and(e.left_child(), e.right_child());
        break;
    case ast::BinaryOperator::LogicalOr:
        gen_logical_or(e.left_child(), e.right_child());
        break;

// Simple binary expression case: compile lhs and rhs, then apply operator.
#define HAMMER_SIMPLE_BINARY(op, opcode)            \
    case ast::BinaryOperator::op:                   \
        func_.generate_expr_value(e.left_child());  \
        func_.generate_expr_value(e.right_child()); \
        builder_.opcode();                          \
        break;

        HAMMER_SIMPLE_BINARY(Plus, add)
        HAMMER_SIMPLE_BINARY(Minus, sub)
        HAMMER_SIMPLE_BINARY(Multiply, mul)
        HAMMER_SIMPLE_BINARY(Divide, div)
        HAMMER_SIMPLE_BINARY(Modulus, mod)
        HAMMER_SIMPLE_BINARY(Power, pow)

        HAMMER_SIMPLE_BINARY(Less, lt)
        HAMMER_SIMPLE_BINARY(LessEquals, lte)
        HAMMER_SIMPLE_BINARY(Greater, gt)
        HAMMER_SIMPLE_BINARY(GreaterEquals, gte)
        HAMMER_SIMPLE_BINARY(Equals, eq)
        HAMMER_SIMPLE_BINARY(NotEquals, neq)

        HAMMER_SIMPLE_BINARY(LeftShift, lsh)
        HAMMER_SIMPLE_BINARY(RightShift, rsh)
        HAMMER_SIMPLE_BINARY(BitwiseAnd, band)
        HAMMER_SIMPLE_BINARY(BitwiseOr, bor)
        HAMMER_SIMPLE_BINARY(BitwiseXor, bxor)
#undef HAMMER_SIMPLE_BINARY
    }
}

void ExprCodegen::gen_impl(ast::VarExpr& e) {
    func_.generate_load(e.decl());
}

void ExprCodegen::gen_impl(ast::DotExpr& e) {
    HAMMER_ASSERT(e.name().valid(), "Invalid member name.");

    // Pushes the object we're accessing.
    func_.generate_expr_value(e.inner());

    // Loads the member of the object.
    const u32 symbol_index = module().add_symbol(e.name());
    builder_.load_member(symbol_index);
}

void ExprCodegen::gen_impl(ast::CallExpr& e) {
    HAMMER_ASSERT_NOT_NULL(e.func());

    if (auto* dot = try_cast<ast::DotExpr>(e.func())) {
        func_.generate_expr_value(dot->inner());

        const u32 symbol_index = module().add_symbol(dot->name());

        const size_t args = e.arg_count();
        for (size_t i = 0; i < args; ++i) {
            ast::Expr* arg = e.get_arg(i);
            func_.generate_expr_value(arg);
        }
        HAMMER_CHECK(
            args <= std::numeric_limits<u32>::max(), "Too many arguments.");
        builder_.call_member(symbol_index, args);
    } else {
        func_.generate_expr_value(e.func());

        const size_t args = e.arg_count();
        for (size_t i = 0; i < args; ++i) {
            ast::Expr* arg = e.get_arg(i);
            func_.generate_expr_value(arg);
        }

        HAMMER_CHECK(
            args <= std::numeric_limits<u32>::max(), "Too many arguments.");
        builder_.call(args);
    }
}

void ExprCodegen::gen_impl(ast::IndexExpr& e) {
    func_.generate_expr_value(e.inner());
    func_.generate_expr_value(e.index());
    builder_.load_index();
}

void ExprCodegen::gen_impl(ast::IfExpr& e) {
    LabelGroup group(builder_);
    const LabelID if_else = group.gen("if-else");
    const LabelID if_end = group.gen("if-end");

    if (!e.else_branch()) {
        HAMMER_ASSERT(
            !e.can_use_as_value(), "If expr cannot have a value with one arm.");

        func_.generate_expr_value(e.condition());
        builder_.jmp_false_pop(if_end);

        func_.generate_expr(e.then_branch());
        if (e.then_branch()->expr_type() == ast::ExprType::Value)
            builder_.pop();

        builder_.define_label(if_end);
    } else {
        func_.generate_expr_value(e.condition());
        builder_.jmp_false_pop(if_else);

        func_.generate_expr(e.then_branch());
        if (e.then_branch()->expr_type() == ast::ExprType::Value
            && e.expr_type() != ast::ExprType::Value)
            builder_.pop();

        builder_.jmp(if_end);

        builder_.define_label(if_else);
        func_.generate_expr(e.else_branch());
        if (e.else_branch()->expr_type() == ast::ExprType::Value
            && e.expr_type() != ast::ExprType::Value)
            builder_.pop();

        builder_.define_label(if_end);
    }
}

void ExprCodegen::gen_impl(ast::ReturnExpr& e) {
    if (e.inner()) {
        func_.generate_expr_value(e.inner());
        if (e.inner()->expr_type() == ast::ExprType::Value)
            builder_.ret();
    } else {
        builder_.load_null();
        builder_.ret();
    }
}

void ExprCodegen::gen_impl(ast::ContinueExpr&) {
    HAMMER_CHECK(func_.current_loop(), "Not in a loop.");
    HAMMER_CHECK(func_.current_loop()->continue_label,
        "Continue label not defined for this loop.");
    builder_.jmp(func_.current_loop()->continue_label);
}

void ExprCodegen::gen_impl(ast::BreakExpr&) {
    HAMMER_CHECK(func_.current_loop(), "Not in a loop.");
    HAMMER_CHECK(func_.current_loop()->break_label,
        "Break label not defined for this loop.");
    builder_.jmp(func_.current_loop()->break_label);
}

void ExprCodegen::gen_impl(ast::BlockExpr& e) {
    const size_t statements = e.stmt_count();

    if (e.can_use_as_value()) {
        HAMMER_CHECK(statements > 0,
            "A block expression that producses a value must have at least one "
            "statement.");

        auto* last = try_cast<ast::ExprStmt>(e.get_stmt(e.stmt_count() - 1));
        HAMMER_CHECK(last,
            "Last statement of expression block must be a expression statement "
            "in this block.");
        HAMMER_CHECK(
            last->used(), "Last statement must have the \"used\" flag set.");
    }

    for (size_t i = 0; i < statements; ++i) {
        func_.generate_stmt(e.get_stmt(i));
    }
}

void ExprCodegen::gen_impl(ast::NullLiteral&) {
    builder_.load_null();
}

void ExprCodegen::gen_impl(ast::BooleanLiteral& e) {
    if (e.value()) {
        builder_.load_true();
    } else {
        builder_.load_false();
    }
}

void ExprCodegen::gen_impl(ast::IntegerLiteral& e) {
    // TODO more instructions (for smaller numbers that dont need 64 bit)
    // and / or use constant table.
    builder_.load_int(e.value());
}

void ExprCodegen::gen_impl(ast::FloatLiteral& e) {
    builder_.load_float(e.value());
}

void ExprCodegen::gen_impl(ast::StringLiteral& e) {
    HAMMER_ASSERT(e.value().valid(), "Invalid string constant.");

    const u32 constant_index = module().add_string(e.value());
    builder_.load_module(constant_index);
}

void ExprCodegen::gen_impl(ast::SymbolLiteral& e) {
    HAMMER_ASSERT(e.value().valid(), "Invalid symbol value.");

    const u32 symbol_index = module().add_symbol(e.value());
    builder_.load_module(symbol_index);
}

void ExprCodegen::gen_impl(ast::ArrayLiteral& e) {
    for (ast::Expr* expr : e.entries())
        func_.generate_expr_value(expr);

    builder_.mk_array(as_u32(e.entry_count()));
}

void ExprCodegen::gen_impl(ast::TupleLiteral& e) {
    for (ast::Expr* expr : e.entries())
        func_.generate_expr_value(expr);

    builder_.mk_tuple(as_u32(e.entry_count()));
}

void ExprCodegen::gen_impl(ast::MapLiteral& e) {
    for (auto [key, value] : e.entries()) {
        func_.generate_expr_value(key);
        func_.generate_expr_value(value);
    }

    builder_.mk_map(as_u32(e.entry_count()));
}

void ExprCodegen::gen_impl(ast::SetLiteral& e) {
    for (ast::Expr* expr : e.entries())
        func_.generate_expr_value(expr);

    builder_.mk_set(as_u32(e.entry_count()));
}

void ExprCodegen::gen_impl(ast::FuncLiteral& e) {
    func_.generate_closure(e.func());
}

void ExprCodegen::gen_assign(ast::BinaryExpr* assign) {
    HAMMER_ASSERT_NOT_NULL(assign);
    HAMMER_ASSERT(assign->operation() == ast::BinaryOperator::Assign,
        "Expression must be an assignment.");

    // TODO: Use optimization at SSA level instead.
    const bool has_value = assign->expr_type() == ast::ExprType::Value;

    auto visitor = Overloaded{//
        [&](ast::DotExpr& e) {
            gen_member_assign(&e, assign->right_child(), has_value);
        },
        [&](ast::IndexExpr& e) {
            gen_index_assign(&e, assign->right_child(), has_value);
        },
        [&](ast::VarExpr& e) {
            func_.generate_store(e.decl(), assign->right_child(), has_value);
        },
        [&](ast::Expr& e) {
            HAMMER_ERROR("Invalid left hand side of type {} in assignment.",
                to_string(e.kind()));
        }};
    ast::visit(*assign->left_child(), visitor);
}

void ExprCodegen::gen_member_assign(
    ast::DotExpr* lhs, ast::Expr* rhs, bool push_value) {
    HAMMER_ASSERT_NOT_NULL(lhs);

    // Pushes the object who's member we're manipulating.
    func_.generate_expr_value(lhs->inner());

    // Pushes the value for the assignment.
    func_.generate_expr_value(rhs);

    if (push_value) {
        builder_.dup();
        builder_.rot_3();
    }

    // Performs the assignment.
    const u32 symbol_index = module().add_symbol(lhs->name());
    builder_.store_member(symbol_index);
}

void ExprCodegen::gen_index_assign(
    ast::IndexExpr* lhs, ast::Expr* rhs, bool push_value) {
    HAMMER_ASSERT_NOT_NULL(lhs);

    // Pushes the object
    func_.generate_expr_value(lhs->inner());

    // Pushes the index value
    func_.generate_expr_value(lhs->index());

    // Pushes the value for the assignment.
    func_.generate_expr_value(rhs);

    if (push_value) {
        builder_.dup();
        builder_.rot_4();
    }

    builder_.store_index();
}

void ExprCodegen::gen_logical_and(ast::Expr* lhs, ast::Expr* rhs) {
    LabelGroup group(builder_);
    const LabelID and_end = group.gen("and-end");

    func_.generate_expr_value(lhs);
    builder_.jmp_false(and_end);

    builder_.pop();
    func_.generate_expr_value(rhs);
    builder_.define_label(and_end);
}

void ExprCodegen::gen_logical_or(ast::Expr* lhs, ast::Expr* rhs) {
    LabelGroup group(builder_);
    const LabelID or_end = group.gen("or-end");

    func_.generate_expr_value(lhs);
    builder_.jmp_true(or_end);

    builder_.pop();
    func_.generate_expr_value(rhs);
    builder_.define_label(or_end);
}

} // namespace hammer
