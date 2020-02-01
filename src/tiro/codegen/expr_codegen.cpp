#include "tiro/codegen/expr_codegen.hpp"

#include "tiro/codegen/func_codegen.hpp"
#include "tiro/codegen/module_codegen.hpp"
#include "tiro/core/overloaded.hpp"

namespace tiro::compiler {

[[noreturn]] static void no_codegen_impl() {
    TIRO_UNREACHABLE(
        "No codegen impl for this type (it should have been lowered earlier).");
}

ExprCodegen::ExprCodegen(
    NotNull<Expr*> expr, CurrentBasicBlock& bb, FunctionCodegen& func)
    : expr_(expr)
    , func_(func)
    , bb_(bb)
    , strings_(func.strings())
    , diag_(func.diag()) {}

bool ExprCodegen::generate() {
    TIRO_ASSERT(!expr_->has_error(), "Invalid expression node.");

    return visit(TIRO_NN(expr_), *this);
}

ModuleCodegen& ExprCodegen::module() {
    return func_.module();
}

bool ExprCodegen::visit_unary_expr(UnaryExpr* e) {
    switch (e->operation()) {
#define TIRO_SIMPLE_OP(op, opcode)                           \
    case UnaryOperator::op:                                  \
        func_.generate_expr_value(TIRO_NN(e->inner()), bb_); \
        bb_->builder().opcode();                             \
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
        return gen_assign(TIRO_NN(e));

    case BinaryOperator::AssignPlus:
    case BinaryOperator::AssignMinus:
    case BinaryOperator::AssignMultiply:
    case BinaryOperator::AssignDivide:
    case BinaryOperator::AssignModulus:
    case BinaryOperator::AssignPower:
        no_codegen_impl();
        return false;

    case BinaryOperator::LogicalAnd:
        gen_logical_and(TIRO_NN(e->left()), TIRO_NN(e->right()));
        return true;
    case BinaryOperator::LogicalOr:
        gen_logical_or(TIRO_NN(e->left()), TIRO_NN(e->right()));
        return true;

// Simple binary expression case: compile lhs and rhs, then apply operator.
#define TIRO_SIMPLE_OP(op, opcode)                           \
    case BinaryOperator::op:                                 \
        func_.generate_expr_value(TIRO_NN(e->left()), bb_);  \
        func_.generate_expr_value(TIRO_NN(e->right()), bb_); \
        bb_->builder().opcode();                             \
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
    func_.generate_load(e->resolved_symbol(), bb_);
    return true;
}

bool ExprCodegen::visit_dot_expr(DotExpr* e) {
    TIRO_ASSERT(e->name().valid(), "Invalid member name.");

    // Pushes the object we're accessing.
    func_.generate_expr_value(TIRO_NN(e->inner()), bb_);

    // Loads the member of the object.
    const u32 symbol_index = module().add_symbol(e->name());
    bb_->builder().load_member(symbol_index);
    return true;
}

bool ExprCodegen::visit_tuple_member_expr(TupleMemberExpr* e) {
    func_.generate_expr_value(TIRO_NN(e->inner()), bb_);
    bb_->builder().load_tuple_member(e->index());
    return true;
}

bool ExprCodegen::visit_call_expr(CallExpr* e) {
    const auto called_function = TIRO_NN(e->func());

    if (auto dot = try_cast<DotExpr>(called_function)) {
        func_.generate_expr_value(TIRO_NN(dot->inner()), bb_);

        const u32 symbol_index = module().add_symbol(dot->name());
        bb_->builder().load_method(symbol_index);

        const auto args = TIRO_NN(e->args());
        for (const auto& arg : args->entries()) {
            func_.generate_expr_value(TIRO_NN(arg), bb_);
        }
        bb_->builder().call_method(checked_cast<u32>(args->size()));
    } else {
        func_.generate_expr_value(called_function, bb_);

        const auto args = TIRO_NN(e->args());
        for (const auto& arg : args->entries()) {
            func_.generate_expr_value(TIRO_NN(arg), bb_);
        }
        bb_->builder().call(checked_cast<u32>(args->size()));
    }
    return true;
}

bool ExprCodegen::visit_index_expr(IndexExpr* e) {
    func_.generate_expr_value(TIRO_NN(e->inner()), bb_);
    func_.generate_expr_value(TIRO_NN(e->index()), bb_);
    bb_->builder().load_index();
    return true;
}

bool ExprCodegen::visit_if_expr(IfExpr* e) {
    const auto& cond = e->condition();
    const auto& then_expr = e->then_branch();
    const auto& else_expr = e->else_branch();

    const bool observed = e->observed();
    const bool has_value = e->expr_type() == ExprType::Value;

    func_.generate_expr_value(TIRO_NN(cond), bb_);

    if (!else_expr) {
        TIRO_ASSERT(!can_use_as_value(e->expr_type()),
            "If expr cannot have a value with one arm.");

        auto then_block = func_.blocks().make_block(strings_.insert("if-then"));
        auto end_block = func_.blocks().make_block(strings_.insert("if-end"));
        bb_->edge(BasicBlockEdge::make_cond_jump(
            Opcode::JmpFalsePop, end_block, then_block));

        // Then branch
        {
            CurrentBasicBlock nested(TIRO_NN(then_block));
            func_.generate_expr_ignore(TIRO_NN(then_expr), nested);
            nested->edge(BasicBlockEdge::make_jump(end_block));
        }

        bb_.assign(TIRO_NN(end_block));
    } else {
        const bool generate = has_value && observed;

        auto then_block = func_.blocks().make_block(strings_.insert("if-then"));
        auto else_block = func_.blocks().make_block(strings_.insert("if-else"));
        auto end_block = func_.blocks().make_block(strings_.insert("if-end"));
        bb_->edge(BasicBlockEdge::make_cond_jump(
            Opcode::JmpFalsePop, else_block, then_block));

        // Then branch
        {
            CurrentBasicBlock nested(TIRO_NN(then_block));
            if (generate) {
                func_.generate_expr_value(TIRO_NN(then_expr), nested);
            } else {
                func_.generate_expr_ignore(TIRO_NN(then_expr), nested);
            }
            nested->edge(BasicBlockEdge::make_jump(end_block));
        }

        // Else branch
        {
            CurrentBasicBlock nested(TIRO_NN(else_block));
            if (generate) {
                func_.generate_expr_value(TIRO_NN(else_expr), nested);
            } else {
                func_.generate_expr_ignore(TIRO_NN(else_expr), nested);
            }
            nested->edge(BasicBlockEdge::make_jump(end_block));
        }

        bb_.assign(TIRO_NN(end_block));
    }

    return observed;
}

bool ExprCodegen::visit_return_expr(ReturnExpr* e) {
    if (const auto& inner = e->inner()) {
        func_.generate_expr_value(TIRO_NN(inner), bb_);

        if (inner->expr_type() == ExprType::Value) {
            bb_->edge(BasicBlockEdge::make_ret());
        } else {
            bb_->edge(BasicBlockEdge::make_never());
        }
    } else {
        bb_->builder().load_null();
        bb_->edge(BasicBlockEdge::make_ret());
    }

    auto unreachable = func_.blocks().make_block(
        strings_.insert("after-return"));
    bb_.assign(TIRO_NN(unreachable));
    return true;
}

bool ExprCodegen::visit_continue_expr([[maybe_unused]] ContinueExpr* e) {
    TIRO_CHECK(func_.current_loop(), "Not in a loop.");
    TIRO_CHECK(func_.current_loop()->continue_label,
        "Continue label not defined for this loop.");

    auto loop = TIRO_NN(func_.current_loop());
    gen_loop_jump(TIRO_NN(loop->continue_label));
    return true;
}

bool ExprCodegen::visit_break_expr([[maybe_unused]] BreakExpr* e) {
    TIRO_CHECK(func_.current_loop(), "Not in a loop.");
    TIRO_CHECK(func_.current_loop()->break_label,
        "Break label not defined for this loop.");

    auto loop = TIRO_NN(func_.current_loop());
    gen_loop_jump(TIRO_NN(loop->break_label));
    return true;
}

bool ExprCodegen::visit_block_expr(BlockExpr* e) {
    const auto stmts = TIRO_NN(e->stmts());

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
        func_.generate_stmt(TIRO_NN(stmts->get(i)), bb_);
    }

    if (produces_value && observed) {
        auto last = try_cast<ExprStmt>(stmts->get(stmts_count - 1));
        TIRO_CHECK(last,
            "Last statement of expression block must be a expression statement "
            "in this block.");
        func_.generate_expr_value(TIRO_NN(last->expr()), bb_);
    }

    return observed;
}

bool ExprCodegen::visit_string_sequence_expr(StringSequenceExpr*) {
    no_codegen_impl();
}

bool ExprCodegen::visit_interpolated_string_expr(InterpolatedStringExpr* e) {
    const auto items = e->items();

    bb_->builder().mk_builder();
    for (auto expr : items->entries()) {
        func_.generate_expr_value(TIRO_NN(expr), bb_);
        bb_->builder().builder_append();
    }
    bb_->builder().builder_string();
    return true;
}

bool ExprCodegen::visit_null_literal(NullLiteral*) {
    bb_->builder().load_null();
    return true;
}

bool ExprCodegen::visit_boolean_literal(BooleanLiteral* e) {
    if (e->value()) {
        bb_->builder().load_true();
    } else {
        bb_->builder().load_false();
    }
    return true;
}

bool ExprCodegen::visit_integer_literal(IntegerLiteral* e) {
    // TODO more instructions (for smaller numbers that dont need 64 bit)
    // and / or use constant table.
    bb_->builder().load_int(e->value());
    return true;
}

bool ExprCodegen::visit_float_literal(FloatLiteral* e) {
    bb_->builder().load_float(e->value());
    return true;
}

bool ExprCodegen::visit_string_literal(StringLiteral* e) {
    TIRO_ASSERT(e->value().valid(), "Invalid string constant.");

    const u32 constant_index = module().add_string(e->value());
    bb_->builder().load_module(constant_index);
    return true;
}

bool ExprCodegen::visit_symbol_literal(SymbolLiteral* e) {
    TIRO_ASSERT(e->value().valid(), "Invalid symbol value.");

    const u32 symbol_index = module().add_symbol(e->value());
    bb_->builder().load_module(symbol_index);
    return true;
}

bool ExprCodegen::visit_array_literal(ArrayLiteral* e) {
    const auto list = TIRO_NN(e->entries());
    for (auto expr : list->entries())
        func_.generate_expr_value(TIRO_NN(expr), bb_);

    bb_->builder().mk_array(checked_cast<u32>(list->size()));
    return true;
}

bool ExprCodegen::visit_tuple_literal(TupleLiteral* e) {
    const auto list = TIRO_NN(e->entries());
    for (auto expr : list->entries())
        func_.generate_expr_value(TIRO_NN(expr), bb_);

    bb_->builder().mk_tuple(checked_cast<u32>(list->size()));
    return true;
}

bool ExprCodegen::visit_map_literal(MapLiteral* e) {
    const auto list = TIRO_NN(e->entries());
    for (auto entry : list->entries()) {
        func_.generate_expr_value(TIRO_NN(entry->key()), bb_);
        func_.generate_expr_value(TIRO_NN(entry->value()), bb_);
    }

    bb_->builder().mk_map(checked_cast<u32>(list->size()));
    return true;
}

bool ExprCodegen::visit_set_literal(SetLiteral* e) {
    const auto list = TIRO_NN(e->entries());
    for (auto expr : list->entries())
        func_.generate_expr_value(TIRO_NN(expr), bb_);

    bb_->builder().mk_set(checked_cast<u32>(list->size()));
    return true;
}

bool ExprCodegen::visit_func_literal(FuncLiteral* e) {
    func_.generate_closure(TIRO_NN(e->func()), bb_);
    return true;
}

bool ExprCodegen::gen_assign(NotNull<BinaryExpr*> assign) {
    TIRO_ASSERT(assign->operation() == BinaryOperator::Assign,
        "Expression must be an assignment.");
    TIRO_ASSERT(assign->expr_type() == ExprType::Value,
        "Invalid expression type for assignment.");

    // TODO: Use optimization at SSA level instead.
    const bool has_value = assign->observed();

    // TODO: If both the left and the right side of an assignment are tuple
    // literals, we can just "assign through" the variables. I.e. (a, b) = (b, a + b)
    // can just be two individual assignments without generating the tuple.
    gen_store(TIRO_NN(assign->left()), TIRO_NN(assign->right()), has_value);
    return has_value;
}

void ExprCodegen::gen_store(
    NotNull<Expr*> lhs, NotNull<Expr*> rhs, bool has_value) {
    auto visitor = Overloaded{
        [&](DotExpr* e) { gen_member_store(TIRO_NN(e), rhs, has_value); },
        [&](TupleMemberExpr* e) {
            gen_tuple_member_store(TIRO_NN(e), rhs, has_value);
        },
        [&](TupleLiteral* e) { gen_tuple_store(TIRO_NN(e), rhs, has_value); },
        [&](IndexExpr* e) { gen_index_store(TIRO_NN(e), rhs, has_value); },
        [&](VarExpr* e) { gen_var_store(TIRO_NN(e), rhs, has_value); },
        [&](Expr* e) {
            TIRO_ERROR("Invalid left hand side of type {} in assignment.",
                to_string(e->type()));
        }};
    downcast(lhs, visitor);
}

void ExprCodegen::gen_var_store(
    NotNull<VarExpr*> lhs, NotNull<Expr*> rhs, bool has_value) {
    func_.generate_expr_value(rhs, bb_);
    if (has_value)
        bb_->builder().dup();

    func_.generate_store(lhs->resolved_symbol(), bb_);
}

void ExprCodegen::gen_member_store(
    NotNull<DotExpr*> lhs, NotNull<Expr*> rhs, bool has_value) {
    // Pushes the object who's member we're manipulating.
    func_.generate_expr_value(TIRO_NN(lhs->inner()), bb_);

    // Generates the assignment operand.
    func_.generate_expr_value(rhs, bb_);
    if (has_value) {
        bb_->builder().dup();
        bb_->builder().rot_3();
    }

    // Performs the assignment.
    const u32 symbol_index = module().add_symbol(lhs->name());
    bb_->builder().store_member(symbol_index);
}

void ExprCodegen::gen_tuple_member_store(
    NotNull<TupleMemberExpr*> lhs, NotNull<Expr*> rhs, bool has_value) {
    // Pushes the tuple who's member we're setting.
    func_.generate_expr_value(TIRO_NN(lhs->inner()), bb_);

    // Generates the assignment operand.
    func_.generate_expr_value(rhs, bb_);
    if (has_value) {
        bb_->builder().dup();
        bb_->builder().rot_3();
    }

    // Assigns the value
    bb_->builder().store_tuple_member(lhs->index());
}

void ExprCodegen::gen_index_store(
    NotNull<IndexExpr*> lhs, NotNull<Expr*> rhs, bool has_value) {
    // Pushes the object
    func_.generate_expr_value(TIRO_NN(lhs->inner()), bb_);

    // Pushes the index value
    func_.generate_expr_value(TIRO_NN(lhs->index()), bb_);

    // Generates the assignment operand.
    func_.generate_expr_value(rhs, bb_);
    if (has_value) {
        bb_->builder().dup();
        bb_->builder().rot_4();
    }

    // Assigns the value to the index
    bb_->builder().store_index();
}

void ExprCodegen::gen_tuple_store([[maybe_unused]] NotNull<TupleLiteral*> lhs,
    [[maybe_unused]] NotNull<Expr*> rhs, [[maybe_unused]] bool has_value) {

    if (lhs->entries()->size() == 0) {
        func_.generate_expr_value(rhs, bb_);
        if (!has_value)
            bb_->builder().pop();
        return;
    }

    struct RecursiveImpl {
        FunctionCodegen& func_;
        CurrentBasicBlock& bb_;
        NotNull<TupleLiteral*> lhs_;
        NotNull<ExprList*> entries_;
        NotNull<Expr*> rhs_;
        bool has_value_;

        explicit RecursiveImpl(FunctionCodegen& func, CurrentBasicBlock& bb,
            NotNull<TupleLiteral*> lhs, NotNull<Expr*> rhs, bool has_value)
            : func_(func)
            , bb_(bb)
            , lhs_(lhs)
            , entries_(TIRO_NN(lhs->entries()))
            , rhs_(rhs)
            , has_value_(has_value) {
            TIRO_ASSERT(
                entries_->size() > 0, "Must have at least one lhs entry.");
            TIRO_ASSERT(entries_->size() <= std::numeric_limits<u32>::max(),
                "Too many tuple elements.");
        }

        // Every invocation generates the current assignment and leaves the tuple value
        // behind at the last value on the stack. The previous invocation
        // will pick up that value and proceed in a similar fashion. This is necessary
        // because the evaluation of the rhs expression must be last (left to right evaluation).
        //
        // It would be more elegant to have a single local that stores the evaluated rhs and is
        // referenced from all invocations but that would need a redesign of the codegen module.
        // With possible changes in the bytecode (stack locations being adressable in opcodes)
        // and a SSA codegen this would be easier.
        //
        // TODO: evaluation of operands is left to right but the assignment itself
        // is from the highest tuple index to the lowest tuple index. This would be observable
        // in exception cases (e.g. array out of bounds write)!
        void gen(u32 tuple_index = 0) {
            TIRO_ASSERT(tuple_index < entries_->size(), "Index out of bounds.");

            auto visitor = Overloaded{//
                [&](VarExpr* expr) {
                    gen_var_assign(TIRO_NN(expr), tuple_index);
                },
                [&](DotExpr* expr) {
                    gen_member_assign(TIRO_NN(expr), tuple_index);
                },
                [&](TupleMemberExpr* expr) {
                    gen_tuple_member_assign(TIRO_NN(expr), tuple_index);
                },
                [&](IndexExpr* expr) {
                    gen_index_assign(TIRO_NN(expr), tuple_index);
                },
                // Note: nested tuple literal assignments not allowed
                [&](Expr* expr) {
                    TIRO_ERROR(
                        "Invalid left hand side of "
                        "type {} in tuple assignment.",
                        to_string(expr->type()));
                }};
            downcast(TIRO_NN(entries_->get(tuple_index)), visitor);
        }

        void gen_var_assign(NotNull<VarExpr*> expr, u32 tuple_index) {
            eval(tuple_index);
            if (has_value_ || tuple_index > 0) {
                bb_->builder().dup();
            }
            bb_->builder().load_tuple_member(tuple_index);

            func_.generate_store(expr->resolved_symbol(), bb_);
        }

        void gen_member_assign(NotNull<DotExpr*> expr, u32 tuple_index) {
            func_.generate_expr_value(TIRO_NN(expr->inner()), bb_);

            eval(tuple_index);
            if (has_value_ || tuple_index > 0) {
                bb_->builder().dup();
                bb_->builder().rot_3();
            }
            bb_->builder().load_tuple_member(tuple_index);

            const u32 symbol_index = func_.module().add_symbol(expr->name());
            bb_->builder().store_member(symbol_index);
        }

        void gen_tuple_member_assign(
            NotNull<TupleMemberExpr*> expr, u32 tuple_index) {
            func_.generate_expr_value(TIRO_NN(expr->inner()), bb_);

            eval(tuple_index);
            if (has_value_ || tuple_index > 0) {
                bb_->builder().dup();
                bb_->builder().rot_3();
            }
            bb_->builder().load_tuple_member(tuple_index);

            bb_->builder().store_tuple_member(expr->index());
        }

        void gen_index_assign(NotNull<IndexExpr*> expr, u32 tuple_index) {
            func_.generate_expr_value(TIRO_NN(expr->inner()), bb_);
            func_.generate_expr_value(TIRO_NN(expr->index()), bb_);

            eval(tuple_index);
            if (has_value_ || tuple_index > 0) {
                bb_->builder().dup();
                bb_->builder().rot_4();
            }
            bb_->builder().load_tuple_member(tuple_index);

            bb_->builder().store_index();
        }

        void eval(u32 tuple_index) {
            if (last(tuple_index)) {
                func_.generate_expr_value(rhs_, bb_);
            } else {
                gen(tuple_index + 1);
            }
        }

        bool last(u32 tuple_index) {
            return tuple_index + 1 == entries_->size();
        }

    } impl(func_, bb_, lhs, rhs, has_value);
    impl.gen();
}

void ExprCodegen::gen_logical_and(NotNull<Expr*> lhs, NotNull<Expr*> rhs) {
    func_.generate_expr_value(lhs, bb_);

    auto then_block = func_.blocks().make_block(strings_.insert("and-then"));
    auto end_block = func_.blocks().make_block(strings_.insert("and-end"));
    bb_->edge(BasicBlockEdge::make_cond_jump(
        Opcode::JmpFalse, end_block, then_block));

    {
        CurrentBasicBlock nested(TIRO_NN(then_block));
        nested->builder().pop();
        func_.generate_expr_value(rhs, nested);

        nested->edge(BasicBlockEdge::make_jump(end_block));
    }

    bb_.assign(TIRO_NN(end_block));
}

void ExprCodegen::gen_logical_or(NotNull<Expr*> lhs, NotNull<Expr*> rhs) {
    func_.generate_expr_value(lhs, bb_);

    auto else_block = func_.blocks().make_block(strings_.insert("or-else"));
    auto end_block = func_.blocks().make_block(strings_.insert("or-end"));
    bb_->edge(
        BasicBlockEdge::make_cond_jump(Opcode::JmpTrue, end_block, else_block));

    {
        CurrentBasicBlock nested(TIRO_NN(else_block));
        nested->builder().pop();
        func_.generate_expr_value(rhs, nested);

        nested->edge(BasicBlockEdge::make_jump(end_block));
    }

    bb_.assign(TIRO_NN(end_block));
}

void ExprCodegen::gen_loop_jump(NotNull<BasicBlock*> target) {
    // FIXME fixup existing values on the stack (balance)
    bb_->edge(BasicBlockEdge::make_jump(target));

    auto unreachable = func_.blocks().make_block(
        strings_.insert("after-loop-jump"));
    bb_.assign(TIRO_NN(unreachable));
}

} // namespace tiro::compiler
