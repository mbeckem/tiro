#include "hammer/compiler/codegen/expr_codegen.hpp"

#include "hammer/core/overloaded.hpp"

namespace hammer::compiler {

[[noreturn]] static void no_codegen_impl() {
    HAMMER_UNREACHABLE(
        "No codegen impl for this type (it should have been lowered earlier).");
}

ExprCodegen::ExprCodegen(const NodePtr<Expr>& expr, FunctionCodegen& func)
    : expr_(expr)
    , func_(func)
    , builder_(func.builder())
    , strings_(func.strings())
    , diag_(func.diag()) {}

void ExprCodegen::generate() {
    HAMMER_ASSERT_NOT_NULL(expr_);
    HAMMER_ASSERT(!expr_->has_error(), "Invalid expression node.");

    visit(expr_, *this);
}

void ExprCodegen::visit_unary_expr(const NodePtr<UnaryExpr>& e) {
    switch (e->operation()) {
#define HAMMER_SIMPLE_OP(op, opcode)           \
    case UnaryOperator::op:                    \
        func_.generate_expr_value(e->inner()); \
        builder_.opcode();                     \
        break;

        HAMMER_SIMPLE_OP(Plus, upos)
        HAMMER_SIMPLE_OP(Minus, uneg);
        HAMMER_SIMPLE_OP(BitwiseNot, bnot);
        HAMMER_SIMPLE_OP(LogicalNot, lnot);
#undef HAMMER_SIMPLE_OP
    }
}

void ExprCodegen::visit_binary_expr(const NodePtr<BinaryExpr>& e) {
    switch (e->operation()) {
    case BinaryOperator::Assign:
        gen_assign(e);
        break;
    case BinaryOperator::LogicalAnd:
        gen_logical_and(e->left(), e->right());
        break;
    case BinaryOperator::LogicalOr:
        gen_logical_or(e->left(), e->right());
        break;

// Simple binary expression case: compile lhs and rhs, then apply operator.
#define HAMMER_SIMPLE_OP(op, opcode)           \
    case BinaryOperator::op:                   \
        func_.generate_expr_value(e->left());  \
        func_.generate_expr_value(e->right()); \
        builder_.opcode();                     \
        break;

        HAMMER_SIMPLE_OP(Plus, add)
        HAMMER_SIMPLE_OP(Minus, sub)
        HAMMER_SIMPLE_OP(Multiply, mul)
        HAMMER_SIMPLE_OP(Divide, div)
        HAMMER_SIMPLE_OP(Modulus, mod)
        HAMMER_SIMPLE_OP(Power, pow)

        HAMMER_SIMPLE_OP(Less, lt)
        HAMMER_SIMPLE_OP(LessEquals, lte)
        HAMMER_SIMPLE_OP(Greater, gt)
        HAMMER_SIMPLE_OP(GreaterEquals, gte)
        HAMMER_SIMPLE_OP(Equals, eq)
        HAMMER_SIMPLE_OP(NotEquals, neq)

        HAMMER_SIMPLE_OP(LeftShift, lsh)
        HAMMER_SIMPLE_OP(RightShift, rsh)
        HAMMER_SIMPLE_OP(BitwiseAnd, band)
        HAMMER_SIMPLE_OP(BitwiseOr, bor)
        HAMMER_SIMPLE_OP(BitwiseXor, bxor)
#undef HAMMER_SIMPLE_OP
    }
}

void ExprCodegen::visit_var_expr(const NodePtr<VarExpr>& e) {
    func_.generate_load(e->resolved_symbol());
}

void ExprCodegen::visit_dot_expr(const NodePtr<DotExpr>& e) {
    HAMMER_ASSERT(e->name().valid(), "Invalid member name.");

    // Pushes the object we're accessing.
    func_.generate_expr_value(e->inner());

    // Loads the member of the object.
    const u32 symbol_index = module().add_symbol(e->name());
    builder_.load_member(symbol_index);
}

void ExprCodegen::visit_call_expr(const NodePtr<CallExpr>& e) {
    HAMMER_ASSERT_NOT_NULL(e->func());

    if (auto dot = try_cast<DotExpr>(e->func())) {
        func_.generate_expr_value(dot->inner());

        const u32 symbol_index = module().add_symbol(dot->name());
        builder_.load_method(symbol_index);

        const auto& args = e->args();
        HAMMER_ASSERT_NOT_NULL(args);

        for (const auto& arg : args->entries()) {
            func_.generate_expr_value(arg);
        }
        builder_.call_method(checked_cast<u32>(args->size()));
    } else {
        func_.generate_expr_value(e->func());

        const auto& args = e->args();
        HAMMER_ASSERT_NOT_NULL(args);

        for (const auto& arg : args->entries()) {
            func_.generate_expr_value(arg);
        }
        builder_.call(checked_cast<u32>(args->size()));
    }
}

void ExprCodegen::visit_index_expr(const NodePtr<IndexExpr>& e) {
    func_.generate_expr_value(e->inner());
    func_.generate_expr_value(e->index());
    builder_.load_index();
}

void ExprCodegen::visit_if_expr(const NodePtr<IfExpr>& e) {
    LabelGroup group(builder_);
    const LabelID if_else = group.gen("if-else");
    const LabelID if_end = group.gen("if-end");

    const auto& cond = e->condition();
    const auto& then_expr = e->then_branch();
    const auto& else_expr = e->else_branch();

    if (!else_expr) {
        HAMMER_ASSERT(!can_use_as_value(e->expr_type()),
            "If expr cannot have a value with one arm.");

        func_.generate_expr_value(cond);
        builder_.jmp_false_pop(if_end);

        func_.generate_expr(then_expr);
        if (then_expr->expr_type() == ExprType::Value)
            builder_.pop();

        builder_.define_label(if_end);
    } else {
        func_.generate_expr_value(cond);
        builder_.jmp_false_pop(if_else);

        func_.generate_expr(then_expr);
        if (then_expr->expr_type() == ExprType::Value
            && e->expr_type() != ExprType::Value)
            builder_.pop();

        builder_.jmp(if_end);

        builder_.define_label(if_else);
        func_.generate_expr(else_expr);
        if (else_expr->expr_type() == ExprType::Value
            && e->expr_type() != ExprType::Value)
            builder_.pop();

        builder_.define_label(if_end);
    }
}

void ExprCodegen::visit_return_expr(const NodePtr<ReturnExpr>& e) {
    if (const auto& inner = e->inner()) {
        func_.generate_expr_value(inner);
        if (inner->expr_type() == ExprType::Value)
            builder_.ret();
    } else {
        builder_.load_null();
        builder_.ret();
    }
}

void ExprCodegen::visit_continue_expr(const NodePtr<ContinueExpr>&) {
    HAMMER_CHECK(func_.current_loop(), "Not in a loop.");
    HAMMER_CHECK(func_.current_loop()->continue_label,
        "Continue label not defined for this loop.");
    builder_.jmp(func_.current_loop()->continue_label);
}

void ExprCodegen::visit_break_expr(const NodePtr<BreakExpr>&) {
    HAMMER_CHECK(func_.current_loop(), "Not in a loop.");
    HAMMER_CHECK(func_.current_loop()->break_label,
        "Break label not defined for this loop.");
    builder_.jmp(func_.current_loop()->break_label);
}

void ExprCodegen::visit_block_expr(const NodePtr<BlockExpr>& e) {
    const auto stmts = e->stmts();
    HAMMER_ASSERT_NOT_NULL(stmts);

    const size_t stmts_count = stmts->size();
    const bool produces_value = can_use_as_value(e->expr_type());

    size_t generated_stmts = stmts_count;
    if (produces_value) {
        HAMMER_CHECK(generated_stmts > 0,
            "A block expression that produces a value must have at least one "
            "statement.");
        generated_stmts--;
    }

    for (size_t i = 0; i < generated_stmts; ++i) {
        func_.generate_stmt(stmts->get(i));
    }

    if (produces_value) {
        auto last = try_cast<ExprStmt>(stmts->get(stmts_count - 1));
        HAMMER_CHECK(last,
            "Last statement of expression block must be a expression statement "
            "in this block.");
        func_.generate_expr_value(last->expr());
    }
}

void ExprCodegen::visit_string_sequence_expr(
    const NodePtr<StringSequenceExpr>&) {
    no_codegen_impl();
}

void ExprCodegen::visit_null_literal(const NodePtr<NullLiteral>&) {
    builder_.load_null();
}

void ExprCodegen::visit_boolean_literal(const NodePtr<BooleanLiteral>& e) {
    if (e->value()) {
        builder_.load_true();
    } else {
        builder_.load_false();
    }
}

void ExprCodegen::visit_integer_literal(const NodePtr<IntegerLiteral>& e) {
    // TODO more instructions (for smaller numbers that dont need 64 bit)
    // and / or use constant table.
    builder_.load_int(e->value());
}

void ExprCodegen::visit_float_literal(const NodePtr<FloatLiteral>& e) {
    builder_.load_float(e->value());
}

void ExprCodegen::visit_string_literal(const NodePtr<StringLiteral>& e) {
    HAMMER_ASSERT(e->value().valid(), "Invalid string constant.");

    const u32 constant_index = module().add_string(e->value());
    builder_.load_module(constant_index);
}

void ExprCodegen::visit_symbol_literal(const NodePtr<SymbolLiteral>& e) {
    HAMMER_ASSERT(e->value().valid(), "Invalid symbol value.");

    const u32 symbol_index = module().add_symbol(e->value());
    builder_.load_module(symbol_index);
}

void ExprCodegen::visit_array_literal(const NodePtr<ArrayLiteral>& e) {
    const auto& list = e->entries();
    HAMMER_ASSERT_NOT_NULL(list);

    for (auto expr : list->entries())
        func_.generate_expr_value(expr);

    builder_.mk_array(checked_cast<u32>(list->size()));
}

void ExprCodegen::visit_tuple_literal(const NodePtr<TupleLiteral>& e) {
    const auto& list = e->entries();
    HAMMER_ASSERT_NOT_NULL(list);

    for (auto expr : list->entries())
        func_.generate_expr_value(expr);

    builder_.mk_tuple(checked_cast<u32>(list->size()));
}

void ExprCodegen::visit_map_literal(const NodePtr<MapLiteral>& e) {
    const auto& list = e->entries();
    HAMMER_ASSERT_NOT_NULL(list);

    for (auto entry : list->entries()) {
        func_.generate_expr_value(entry->key());
        func_.generate_expr_value(entry->value());
    }

    builder_.mk_map(checked_cast<u32>(list->size()));
}

void ExprCodegen::visit_set_literal(const NodePtr<SetLiteral>& e) {
    const auto& list = e->entries();
    HAMMER_ASSERT_NOT_NULL(list);

    for (auto expr : list->entries())
        func_.generate_expr_value(expr);

    builder_.mk_set(checked_cast<u32>(list->size()));
}

void ExprCodegen::visit_func_literal(const NodePtr<FuncLiteral>& e) {
    func_.generate_closure(e->func());
}

void ExprCodegen::gen_assign(const NodePtr<BinaryExpr>& assign) {
    HAMMER_ASSERT_NOT_NULL(assign);
    HAMMER_ASSERT(assign->operation() == BinaryOperator::Assign,
        "Expression must be an assignment.");

    // TODO: Use optimization at SSA level instead.
    const bool has_value = assign->expr_type() == ExprType::Value;

    auto visitor = Overloaded{//
        [&](const NodePtr<DotExpr>& e) {
            gen_member_assign(e, assign->right(), has_value);
        },
        [&](const NodePtr<IndexExpr>& e) {
            gen_index_assign(e, assign->right(), has_value);
        },
        [&](const NodePtr<VarExpr>& e) {
            func_.generate_store(
                e->resolved_symbol(), assign->right(), has_value);
        },
        [&](const NodePtr<Expr>& e) {
            HAMMER_ERROR("Invalid left hand side of type {} in assignment.",
                to_string(e->type()));
        }};
    downcast(assign->left(), visitor);
}

void ExprCodegen::gen_member_assign(
    const NodePtr<DotExpr>& lhs, const NodePtr<Expr>& rhs, bool push_value) {
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
    const NodePtr<IndexExpr>& lhs, const NodePtr<Expr>& rhs, bool push_value) {
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

void ExprCodegen::gen_logical_and(
    const NodePtr<Expr>& lhs, const NodePtr<Expr>& rhs) {
    LabelGroup group(builder_);
    const LabelID and_end = group.gen("and-end");

    func_.generate_expr_value(lhs);
    builder_.jmp_false(and_end);

    builder_.pop();
    func_.generate_expr_value(rhs);
    builder_.define_label(and_end);
}

void ExprCodegen::gen_logical_or(
    const NodePtr<Expr>& lhs, const NodePtr<Expr>& rhs) {
    LabelGroup group(builder_);
    const LabelID or_end = group.gen("or-end");

    func_.generate_expr_value(lhs);
    builder_.jmp_true(or_end);

    builder_.pop();
    func_.generate_expr_value(rhs);
    builder_.define_label(or_end);
}

} // namespace hammer::compiler
