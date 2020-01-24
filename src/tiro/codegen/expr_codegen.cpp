#include "tiro/codegen/expr_codegen.hpp"

#include "tiro/core/overloaded.hpp"

namespace tiro::compiler {

[[noreturn]] static void no_codegen_impl() {
    TIRO_UNREACHABLE(
        "No codegen impl for this type (it should have been lowered earlier).");
}

ExprCodegen::ExprCodegen(Expr* expr, FunctionCodegen& func)
    : expr_(expr)
    , func_(func)
    , builder_(func.builder())
    , strings_(func.strings())
    , diag_(func.diag()) {}

bool ExprCodegen::generate() {
    TIRO_ASSERT_NOT_NULL(expr_);
    TIRO_ASSERT(!expr_->has_error(), "Invalid expression node.");

    return visit(expr_, *this);
}

bool ExprCodegen::visit_unary_expr(UnaryExpr* e) {
    switch (e->operation()) {
#define TIRO_SIMPLE_OP(op, opcode)             \
    case UnaryOperator::op:                    \
        func_.generate_expr_value(e->inner()); \
        builder_.opcode();                     \
        return true;

        TIRO_SIMPLE_OP(Plus, upos)
        TIRO_SIMPLE_OP(Minus, uneg);
        TIRO_SIMPLE_OP(BitwiseNot, bnot);
        TIRO_SIMPLE_OP(LogicalNot, lnot);
#undef TIRO_SIMPLE_OP
    }

    TIRO_UNREACHABLE("Invalid binary operation type.");
}

bool ExprCodegen::visit_binary_expr(BinaryExpr* e) {
    switch (e->operation()) {
    case BinaryOperator::Assign:
        return gen_assign(e);
    case BinaryOperator::LogicalAnd:
        gen_logical_and(e->left(), e->right());
        return true;
    case BinaryOperator::LogicalOr:
        gen_logical_or(e->left(), e->right());
        return true;

// Simple binary expression case: compile lhs and rhs, then apply operator.
#define TIRO_SIMPLE_OP(op, opcode)             \
    case BinaryOperator::op:                   \
        func_.generate_expr_value(e->left());  \
        func_.generate_expr_value(e->right()); \
        builder_.opcode();                     \
        return true;

        TIRO_SIMPLE_OP(Plus, add)
        TIRO_SIMPLE_OP(Minus, sub)
        TIRO_SIMPLE_OP(Multiply, mul)
        TIRO_SIMPLE_OP(Divide, div)
        TIRO_SIMPLE_OP(Modulus, mod)
        TIRO_SIMPLE_OP(Power, pow)

        TIRO_SIMPLE_OP(Less, lt)
        TIRO_SIMPLE_OP(LessEquals, lte)
        TIRO_SIMPLE_OP(Greater, gt)
        TIRO_SIMPLE_OP(GreaterEquals, gte)
        TIRO_SIMPLE_OP(Equals, eq)
        TIRO_SIMPLE_OP(NotEquals, neq)

        TIRO_SIMPLE_OP(LeftShift, lsh)
        TIRO_SIMPLE_OP(RightShift, rsh)
        TIRO_SIMPLE_OP(BitwiseAnd, band)
        TIRO_SIMPLE_OP(BitwiseOr, bor)
        TIRO_SIMPLE_OP(BitwiseXor, bxor)
#undef TIRO_SIMPLE_OP
    }

    TIRO_UNREACHABLE("Invalid binary operation type.");
}

bool ExprCodegen::visit_var_expr(VarExpr* e) {
    func_.generate_load(e->resolved_symbol());
    return true;
}

bool ExprCodegen::visit_dot_expr(DotExpr* e) {
    TIRO_ASSERT(e->name().valid(), "Invalid member name.");

    // Pushes the object we're accessing.
    func_.generate_expr_value(e->inner());

    // Loads the member of the object.
    const u32 symbol_index = module().add_symbol(e->name());
    builder_.load_member(symbol_index);
    return true;
}

bool ExprCodegen::visit_tuple_member_expr(TupleMemberExpr* e) {
    func_.generate_expr_value(e->inner());
    builder_.load_tuple_member(e->index());
    return true;
}

bool ExprCodegen::visit_call_expr(CallExpr* e) {
    TIRO_ASSERT_NOT_NULL(e->func());

    if (auto dot = try_cast<DotExpr>(e->func())) {
        func_.generate_expr_value(dot->inner());

        const u32 symbol_index = module().add_symbol(dot->name());
        builder_.load_method(symbol_index);

        const auto& args = e->args();
        TIRO_ASSERT_NOT_NULL(args);

        for (const auto& arg : args->entries()) {
            func_.generate_expr_value(arg);
        }
        builder_.call_method(checked_cast<u32>(args->size()));
    } else {
        func_.generate_expr_value(e->func());

        const auto& args = e->args();
        TIRO_ASSERT_NOT_NULL(args);

        for (const auto& arg : args->entries()) {
            func_.generate_expr_value(arg);
        }
        builder_.call(checked_cast<u32>(args->size()));
    }
    return true;
}

bool ExprCodegen::visit_index_expr(IndexExpr* e) {
    func_.generate_expr_value(e->index());
    func_.generate_expr_value(e->inner());
    builder_.load_index();
    return true;
}

bool ExprCodegen::visit_if_expr(IfExpr* e) {
    LabelGroup group(builder_);
    const LabelID if_else = group.gen("if-else");
    const LabelID if_end = group.gen("if-end");

    const auto& cond = e->condition();
    const auto& then_expr = e->then_branch();
    const auto& else_expr = e->else_branch();

    const bool observed = e->observed();
    const bool has_value = e->expr_type() == ExprType::Value;

    if (!else_expr) {
        TIRO_ASSERT(!can_use_as_value(e->expr_type()),
            "If expr cannot have a value with one arm.");

        func_.generate_expr_value(cond);
        builder_.jmp_false_pop(if_end);

        func_.generate_expr_ignore(then_expr);

        builder_.define_label(if_end);
    } else {
        func_.generate_expr_value(cond);
        builder_.jmp_false_pop(if_else);

        if (has_value && observed) {
            func_.generate_expr_value(then_expr);
        } else {
            func_.generate_expr_ignore(then_expr);
        }
        builder_.jmp(if_end);

        builder_.define_label(if_else);
        if (has_value && observed) {
            func_.generate_expr_value(else_expr);
        } else {
            func_.generate_expr_ignore(else_expr);
        }

        builder_.define_label(if_end);
    }

    return observed;
}

bool ExprCodegen::visit_return_expr(ReturnExpr* e) {
    if (const auto& inner = e->inner()) {
        func_.generate_expr_value(inner);
        if (inner->expr_type() == ExprType::Value) {
            builder_.ret();
        }
    } else {
        builder_.load_null();
        builder_.ret();
    }
    return true;
}

bool ExprCodegen::visit_continue_expr(ContinueExpr*) {
    TIRO_CHECK(func_.current_loop(), "Not in a loop.");
    TIRO_CHECK(func_.current_loop()->continue_label,
        "Continue label not defined for this loop.");
    builder_.jmp(func_.current_loop()->continue_label);
    return true;
}

bool ExprCodegen::visit_break_expr(BreakExpr*) {
    TIRO_CHECK(func_.current_loop(), "Not in a loop.");
    TIRO_CHECK(func_.current_loop()->break_label,
        "Break label not defined for this loop.");
    builder_.jmp(func_.current_loop()->break_label);
    return true;
}

bool ExprCodegen::visit_block_expr(BlockExpr* e) {
    const auto stmts = e->stmts();
    TIRO_ASSERT_NOT_NULL(stmts);

    const size_t stmts_count = stmts->size();
    const bool produces_value = can_use_as_value(e->expr_type());
    const bool observed = e->observed();

    size_t generated_stmts = stmts_count;
    if (produces_value && observed) {
        TIRO_CHECK(generated_stmts > 0,
            "A block expression that produces a value must have at least one "
            "statement.");
        generated_stmts--;
    }

    for (size_t i = 0; i < generated_stmts; ++i) {
        func_.generate_stmt(stmts->get(i));
    }

    if (produces_value && observed) {
        auto last = try_cast<ExprStmt>(stmts->get(stmts_count - 1));
        TIRO_CHECK(last,
            "Last statement of expression block must be a expression statement "
            "in this block.");
        func_.generate_expr_value(last->expr());
    }

    return observed;
}

bool ExprCodegen::visit_string_sequence_expr(StringSequenceExpr*) {
    no_codegen_impl();
}

bool ExprCodegen::visit_interpolated_string_expr(InterpolatedStringExpr* e) {
    const auto items = e->items();

    builder_.mk_builder();
    for (auto expr : items->entries()) {
        func_.generate_expr_value(expr);
        builder_.builder_append();
    }
    builder_.builder_string();
    return true;
}

bool ExprCodegen::visit_null_literal(NullLiteral*) {
    builder_.load_null();
    return true;
}

bool ExprCodegen::visit_boolean_literal(BooleanLiteral* e) {
    if (e->value()) {
        builder_.load_true();
    } else {
        builder_.load_false();
    }
    return true;
}

bool ExprCodegen::visit_integer_literal(IntegerLiteral* e) {
    // TODO more instructions (for smaller numbers that dont need 64 bit)
    // and / or use constant table.
    builder_.load_int(e->value());
    return true;
}

bool ExprCodegen::visit_float_literal(FloatLiteral* e) {
    builder_.load_float(e->value());
    return true;
}

bool ExprCodegen::visit_string_literal(StringLiteral* e) {
    TIRO_ASSERT(e->value().valid(), "Invalid string constant.");

    const u32 constant_index = module().add_string(e->value());
    builder_.load_module(constant_index);
    return true;
}

bool ExprCodegen::visit_symbol_literal(SymbolLiteral* e) {
    TIRO_ASSERT(e->value().valid(), "Invalid symbol value.");

    const u32 symbol_index = module().add_symbol(e->value());
    builder_.load_module(symbol_index);
    return true;
}

bool ExprCodegen::visit_array_literal(ArrayLiteral* e) {
    const auto& list = e->entries();
    TIRO_ASSERT_NOT_NULL(list);

    for (auto expr : list->entries())
        func_.generate_expr_value(expr);

    builder_.mk_array(checked_cast<u32>(list->size()));
    return true;
}

bool ExprCodegen::visit_tuple_literal(TupleLiteral* e) {
    const auto& list = e->entries();
    TIRO_ASSERT_NOT_NULL(list);

    for (auto expr : list->entries())
        func_.generate_expr_value(expr);

    builder_.mk_tuple(checked_cast<u32>(list->size()));
    return true;
}

bool ExprCodegen::visit_map_literal(MapLiteral* e) {
    const auto& list = e->entries();
    TIRO_ASSERT_NOT_NULL(list);

    for (auto entry : list->entries()) {
        func_.generate_expr_value(entry->key());
        func_.generate_expr_value(entry->value());
    }

    builder_.mk_map(checked_cast<u32>(list->size()));
    return true;
}

bool ExprCodegen::visit_set_literal(SetLiteral* e) {
    const auto& list = e->entries();
    TIRO_ASSERT_NOT_NULL(list);

    for (auto expr : list->entries())
        func_.generate_expr_value(expr);

    builder_.mk_set(checked_cast<u32>(list->size()));
    return true;
}

bool ExprCodegen::visit_func_literal(FuncLiteral* e) {
    func_.generate_closure(e->func());
    return true;
}

bool ExprCodegen::gen_assign(BinaryExpr* assign) {
    TIRO_ASSERT_NOT_NULL(assign);
    TIRO_ASSERT(assign->operation() == BinaryOperator::Assign,
        "Expression must be an assignment.");
    TIRO_ASSERT(assign->expr_type() == ExprType::Value,
        "Invalid expression type for assignment.");

    // TODO: Use optimization at SSA level instead.
    const bool has_value = assign->observed();

    // TODO: If both the left and the right side of an assignment are tuple
    // literals, we can just "assign through" the variables. I.e. (a, b) = (b, a + b)
    // can just be two individual assignments without generating the tuple.
    func_.generate_expr_value(assign->right());
    if (has_value) {
        builder_.dup();
    }

    gen_store(assign->left());
    return has_value;
}

void ExprCodegen::gen_store(Expr* lhs) {
    TIRO_ASSERT_NOT_NULL(lhs);

    auto visitor = Overloaded{[&](DotExpr* e) { gen_member_store(e); },
        [&](TupleMemberExpr* e) { gen_tuple_member_store(e); },
        [&](TupleLiteral* e) { gen_tuple_store(e); },
        [&](IndexExpr* e) { gen_index_store(e); },
        [&](VarExpr* e) { func_.generate_store(e->resolved_symbol()); },
        [&](Expr* e) {
            TIRO_ERROR("Invalid left hand side of type {} in assignment.",
                to_string(e->type()));
        }};
    downcast(lhs, visitor);
}

void ExprCodegen::gen_member_store(DotExpr* lhs) {
    TIRO_ASSERT_NOT_NULL(lhs);

    // Pushes the object who's member we're manipulating.
    func_.generate_expr_value(lhs->inner());

    // Performs the assignment.
    const u32 symbol_index = module().add_symbol(lhs->name());
    builder_.store_member(symbol_index);
}

void ExprCodegen::gen_tuple_member_store(TupleMemberExpr* lhs) {
    TIRO_ASSERT_NOT_NULL(lhs);

    // Pushes the tuple who's member we're setting.
    func_.generate_expr_value(lhs->inner());

    // Assigns the value
    builder_.store_tuple_member(lhs->index());
}

void ExprCodegen::gen_tuple_store(TupleLiteral* lhs) {
    TIRO_ASSERT_NOT_NULL(lhs);

    // The right hand side tuple value is on top of the stack once.
    // We must dup it for every additional assignment.
    const auto items = lhs->entries();

    const size_t item_count = items->size();
    if (item_count == 0) {
        // 0 variables on the left hand side. side effects already happened
        // so we can just discard the value here.
        builder_.pop();
        return;
    }

    for (size_t i = 0; i < item_count; ++i) {
        if (i != item_count - 1) {
            builder_.dup();
        }
        builder_.load_tuple_member(i);
        gen_store(items->get(i));
    }
}

void ExprCodegen::gen_index_store(IndexExpr* lhs) {
    TIRO_ASSERT_NOT_NULL(lhs);

    // Pushes the index value
    func_.generate_expr_value(lhs->index());

    // Pushes the object
    func_.generate_expr_value(lhs->inner());
    builder_.store_index();
}

void ExprCodegen::gen_logical_and(Expr* lhs, Expr* rhs) {
    LabelGroup group(builder_);
    const LabelID and_end = group.gen("and-end");

    func_.generate_expr_value(lhs);
    builder_.jmp_false(and_end);

    builder_.pop();
    func_.generate_expr_value(rhs);
    builder_.define_label(and_end);
}

void ExprCodegen::gen_logical_or(Expr* lhs, Expr* rhs) {
    LabelGroup group(builder_);
    const LabelID or_end = group.gen("or-end");

    func_.generate_expr_value(lhs);
    builder_.jmp_true(or_end);

    builder_.pop();
    func_.generate_expr_value(rhs);
    builder_.define_label(or_end);
}

} // namespace tiro::compiler