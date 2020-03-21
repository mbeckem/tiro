#include "tiro/mir_gen/gen_expr.hpp"

#include "tiro/mir/types.hpp"
#include "tiro/mir_gen/gen_module.hpp"
#include "tiro/semantics/symbol_table.hpp"

namespace tiro {

namespace {

class AssignmentVisitor : public DefaultNodeVisitor<AssignmentVisitor> {
public:
    AssignmentVisitor(ExprMIRGen* self, NotNull<Expr*> rhs)
        : self_(self)
        , rhs_(rhs)
        , result_(unreachable) {}

    ExprResult&& take_result() { return std::move(result_); }

    void visit_dot_expr(DotExpr* lhs) TIRO_VISITOR_OVERRIDE {
        simple_assign(lhs);
    }

    void visit_tuple_member_expr(TupleMemberExpr* lhs) TIRO_VISITOR_OVERRIDE {
        simple_assign(lhs);
    }

    void visit_index_expr(IndexExpr* lhs) TIRO_VISITOR_OVERRIDE {
        simple_assign(lhs);
    }

    void visit_var_expr(VarExpr* lhs) TIRO_VISITOR_OVERRIDE {
        simple_assign(lhs);
    }

    void visit_tuple_literal(TupleLiteral* lhs) TIRO_VISITOR_OVERRIDE {
        const u32 target_count = lhs->entries()->size();

        // Left to right evaluation order.
        // TODO: Small vec.
        std::vector<AssignTarget> targets;
        targets.reserve(target_count);
        for (u32 i = 0; i < target_count; ++i) {
            auto expr = TIRO_NN(lhs->entries()->get(i));
            auto target = tuple_target_for(expr);
            if (!target)
                return complete(target.failure());

            targets.push_back(*target);
        }

        auto rhs_result = compile_rhs();
        if (!rhs_result)
            return complete(rhs_result);

        for (u32 i = 0; i < target_count; ++i) {
            auto element = bb().compile_rvalue(
                RValue::UseLValue{LValue::make_tuple_field(*rhs_result, i)});
            bb().compile_assign(targets[i], element);
        }

        return complete(rhs_result);
    }

    void visit_expr(Expr* lhs) TIRO_VISITOR_OVERRIDE {
        TIRO_ERROR("Invalid left hand side of type {} in assignment.",
            to_string(lhs->type()));
    }

private:
    template<typename T>
    void simple_assign(T* lhs) {
        auto target = target_for(TIRO_NN(lhs));
        if (!target)
            return complete(target.failure());

        auto rhs_result = compile_rhs();
        if (!rhs_result)
            return complete(rhs_result);

        bb().compile_assign(*target, *rhs_result);
        return complete(rhs_result);
    }

    TransformResult<AssignTarget> tuple_target_for(NotNull<Expr*> node) {
        struct Visitor : DefaultNodeVisitor<Visitor> {
            AssignmentVisitor* assign;
            TransformResult<AssignTarget> result = unreachable;

            Visitor(AssignmentVisitor* assign_)
                : assign(assign_) {}

            void visit_dot_expr(DotExpr* expr) TIRO_VISITOR_OVERRIDE {
                result = assign->target_for(TIRO_NN(expr));
            }

            void visit_tuple_member_expr(
                TupleMemberExpr* expr) TIRO_VISITOR_OVERRIDE {
                result = assign->target_for(TIRO_NN(expr));
            }

            void visit_index_expr(IndexExpr* expr) TIRO_VISITOR_OVERRIDE {
                result = assign->target_for(TIRO_NN(expr));
            }

            void visit_var_expr(VarExpr* expr) TIRO_VISITOR_OVERRIDE {
                result = assign->target_for(TIRO_NN(expr));
            }

            void visit_node(Node* node) TIRO_VISITOR_OVERRIDE {
                TIRO_ERROR(
                    "Invalid left hand side of type {} in tuple assignment.",
                    to_string(node->type()));
            }
        };

        Visitor visitor{this};
        visit(node, visitor);
        return visitor.result;
    }

    TransformResult<AssignTarget> target_for(NotNull<DotExpr*> expr) {
        auto obj_result = bb().compile_expr(TIRO_NN(expr->inner()));
        if (!obj_result)
            return obj_result.failure();

        auto lvalue = LValue::make_field(*obj_result, expr->name());
        return AssignTarget::make_lvalue(lvalue);
    }

    TransformResult<AssignTarget> target_for(NotNull<TupleMemberExpr*> expr) {
        auto obj_result = bb().compile_expr(TIRO_NN(expr->inner()));
        if (!obj_result)
            return obj_result.failure();

        auto lvalue = LValue::make_tuple_field(*obj_result, expr->index());
        return AssignTarget::make_lvalue(lvalue);
    }

    TransformResult<AssignTarget> target_for(NotNull<IndexExpr*> expr) {
        auto array_result = bb().compile_expr(TIRO_NN(expr->inner()));
        if (!array_result)
            return array_result.failure();

        auto index_result = bb().compile_expr(TIRO_NN(expr->index()));
        if (!index_result)
            return index_result.failure();

        auto lvalue = LValue::make_index(*array_result, *index_result);
        return AssignTarget::make_lvalue(lvalue);
    }

    TransformResult<AssignTarget> target_for(NotNull<VarExpr*> expr) {
        // FIXME: symbol table and ast memory management
        auto symbol = expr->resolved_symbol();
        return AssignTarget::make_symbol(TIRO_NN(symbol.get()));
    }

    void complete(ExprResult result) { result_ = std::move(result); }

    ExprResult compile_rhs() { return bb().compile_expr(rhs_); }

    CurrentBlock& bb() const { return self_->bb(); }

private:
    ExprMIRGen* self_;
    NotNull<Expr*> rhs_;
    ExprResult result_;
};

} // namespace

ExprMIRGen::ExprMIRGen(FunctionMIRGen& ctx, CurrentBlock& bb)
    : Transformer(ctx, bb) {}

ExprResult ExprMIRGen::dispatch(NotNull<Expr*> expr) {
    TIRO_ASSERT(!expr->has_error(),
        "Nodes with errors must not reach the mir transformation stage.");
    return visit(expr, *this);
}

ExprResult ExprMIRGen::visit_binary_expr(BinaryExpr* expr) {
    switch (expr->operation()) {
    case BinaryOperator::Assign:
        return compile_assign(TIRO_NN(expr->left()), TIRO_NN(expr->right()));
    case BinaryOperator::LogicalOr:
        return compile_or(TIRO_NN(expr->left()), TIRO_NN(expr->right()));
    case BinaryOperator::LogicalAnd:
        return compile_and(TIRO_NN(expr->left()), TIRO_NN(expr->right()));
    default:
        break;
    }

    auto op = map_operator(expr->operation());
    auto lhs = bb().compile_expr(TIRO_NN(expr->left()));
    if (!lhs)
        return lhs;

    auto rhs = bb().compile_expr(TIRO_NN(expr->right()));
    if (!rhs)
        return rhs;

    return bb().compile_rvalue(RValue::make_binary_op(op, *lhs, *rhs));
}

ExprResult ExprMIRGen::visit_block_expr(BlockExpr* expr) {
    auto stmts = TIRO_NN(expr->stmts());

    const bool has_value = can_use_as_value(expr->expr_type());
    TIRO_CHECK(!has_value || stmts->size() > 0,
        "A block expression that produces a value must have at least one "
        "statement.");

    const size_t plain_stmts = stmts->size() - (has_value ? 1 : 0);
    for (size_t i = 0; i < plain_stmts; ++i) {
        auto result = bb().compile_stmt(TIRO_NN(stmts->get(i)));
        if (!result)
            return result.failure();
    }

    if (has_value) {
        auto last = try_cast<ExprStmt>(stmts->get(plain_stmts));
        TIRO_CHECK(last,
            "The last statement must be an expression statement because "
            "this block produces a value.");
        return bb().compile_expr(TIRO_NN(last->expr()));
    }

    // Blocks without a value don't return a local. This would be safer
    // if we had a real type system.
    return LocalID();
}

ExprResult ExprMIRGen::visit_break_expr([[maybe_unused]] BreakExpr* expr) {
    auto loop = current_loop();
    TIRO_CHECK(loop, "Break outside a loop.");

    auto target = loop->jump_break;
    TIRO_ASSERT(target, "Current loop has an invalid break label.");
    bb().end(Terminator::make_jump(target));
    return unreachable;
}

ExprResult ExprMIRGen::visit_call_expr(CallExpr* expr) {
    const auto func = TIRO_NN(expr->func());

    // This is a member function invocation, i.e. a.b(...).
    if (auto dot = try_cast<DotExpr>(func)) {
        auto object = bb().compile_expr(TIRO_NN(dot->inner()));
        if (!object)
            return object;

        auto method = bb().compile_rvalue(
            RValue::make_method_handle(*object, dot->name()));

        auto args = compile_exprs(TIRO_NN(expr->args()));
        if (!args)
            return args.failure();
        return bb().compile_rvalue(RValue::make_method_call(method, *args));
    }

    // Otherwise: plain old function call.
    auto func_local = bb().compile_expr(func);
    if (!func_local)
        return func_local;

    auto args = compile_exprs(TIRO_NN(expr->args()));
    if (!args)
        return args.failure();

    return bb().compile_rvalue(RValue::make_call(*func_local, *args));
}

ExprResult
ExprMIRGen::visit_continue_expr([[maybe_unused]] ContinueExpr* expr) {
    auto loop = current_loop();
    TIRO_CHECK(loop, "Continue outside a loop.");

    auto target = loop->jump_continue;
    TIRO_ASSERT(target, "Current loop has an invalid break label.");
    bb().end(Terminator::make_jump(target));
    return unreachable;
}

ExprResult ExprMIRGen::visit_dot_expr(DotExpr* expr) {
    TIRO_ASSERT(expr->name().valid(), "Invalid member name.");

    auto inner = bb().compile_expr(TIRO_NN(expr->inner()));
    if (!inner)
        return inner;

    auto lvalue = LValue::make_field(*inner, expr->name());
    return bb().compile_rvalue(RValue::make_use_lvalue(lvalue));
}

ExprResult ExprMIRGen::visit_if_expr(IfExpr* expr) {
    const bool has_value = can_use_as_value(expr->expr_type());

    const auto cond_result = bb().compile_expr(TIRO_NN(expr->condition()));
    if (!cond_result)
        return cond_result;

    if (!expr->else_branch()) {
        TIRO_ASSERT(
            !has_value, "If expr cannot have a value without an else-branch.");

        auto then_block = ctx().make_block(strings().insert("if-then"));
        auto end_block = ctx().make_block(strings().insert("if-end"));
        bb().end(Terminator::make_branch(
            BranchType::IfTrue, *cond_result, then_block, end_block));
        ctx().seal(then_block);

        // Evaluate the then-branch. The result does not matter because the expr is not used as a value.
        [&]() {
            auto nested = ctx().make_current(then_block);
            auto result = nested.compile_expr(
                TIRO_NN(expr->then_branch()), ExprOptions::MaybeInvalid);
            if (!result)
                return;

            nested.end(Terminator::make_jump(end_block));
            return;
        }();

        ctx().seal(end_block);
        bb().assign(end_block);
        return LocalID();
    }

    auto then_block = ctx().make_block(strings().insert("if-then"));
    auto else_block = ctx().make_block(strings().insert("if-else"));
    auto end_block = ctx().make_block(strings().insert("if-end"));
    bb().end(Terminator::make_branch(
        BranchType::IfTrue, *cond_result, then_block, else_block));
    ctx().seal(then_block);
    ctx().seal(else_block);

    const auto expr_options = has_value ? ExprOptions::Default
                                        : ExprOptions::MaybeInvalid;
    auto compile_branch = [&](BlockID block, NotNull<Expr*> branch) {
        auto nested = ctx().make_current(block);
        auto branch_result = nested.compile_expr(branch, expr_options);
        if (!branch_result)
            return branch_result;

        nested.end(Terminator::make_jump(end_block));
        return branch_result;
    };

    auto then_result = compile_branch(then_block, TIRO_NN(expr->then_branch()));
    auto else_result = compile_branch(else_block, TIRO_NN(expr->else_branch()));

    ctx().seal(end_block);
    bb().assign(end_block);

    if (!has_value || !expr->observed())
        return LocalID();
    if (!then_result)
        return else_result;
    if (!else_result)
        return then_result;

    // Avoid trivial phi nodes.
    if (*then_result == *else_result) {
        return *then_result;
    }

    auto phi_id = result().make(Phi{*then_result, *else_result});
    return bb().compile_rvalue(RValue::make_phi(phi_id));
}

ExprResult ExprMIRGen::visit_index_expr(IndexExpr* expr) {
    auto inner = bb().compile_expr(TIRO_NN(expr->inner()));
    if (!inner)
        return inner;

    auto index = bb().compile_expr(TIRO_NN(expr->index()));
    if (!index)
        return index;

    auto lvalue = LValue::make_index(*inner, *index);
    return bb().compile_rvalue(RValue::make_use_lvalue(lvalue));
}

ExprResult
ExprMIRGen::visit_interpolated_string_expr(InterpolatedStringExpr* expr) {
    const auto items = compile_exprs(TIRO_NN(expr->items()));
    if (!items)
        return items.failure();

    return bb().compile_rvalue(RValue::make_format(*items));
}

ExprResult ExprMIRGen::visit_array_literal(ArrayLiteral* expr) {
    auto items = compile_exprs(TIRO_NN(expr->entries()));
    if (!items)
        return items.failure();

    return bb().compile_rvalue(
        RValue::make_container(ContainerType::Array, *items));
}

ExprResult ExprMIRGen::visit_boolean_literal(BooleanLiteral* expr) {
    auto value = expr->value() ? Constant::make_true() : Constant::make_false();
    return bb().compile_rvalue(value);
}

ExprResult ExprMIRGen::visit_float_literal(FloatLiteral* expr) {
    auto constant = Constant::make_float(expr->value());
    return bb().compile_rvalue(constant);
}

ExprResult ExprMIRGen::visit_func_literal(FuncLiteral* expr) {
    auto func = TIRO_NN(expr->func());
    auto envs = ctx().envs();
    auto env = ctx().current_env();

    ModuleMemberID func_id = ctx().module().add_function(func, envs, env);
    auto lvalue = LValue::make_module(func_id);
    auto func_local = bb().compile_rvalue(RValue::make_use_lvalue(lvalue));

    if (env) {
        auto env_id = bb().compile_env(env);
        return bb().compile_rvalue(
            RValue::make_make_closure(env_id, func_local));
    }
    return func_local;
}

ExprResult ExprMIRGen::visit_integer_literal(IntegerLiteral* expr) {
    auto constant = Constant::make_integer(expr->value());
    return bb().compile_rvalue(constant);
}

ExprResult ExprMIRGen::visit_map_literal(MapLiteral* expr) {
    LocalList pairs;
    for (const auto entry : TIRO_NN(expr->entries())->entries()) {
        auto key = bb().compile_expr(TIRO_NN(entry->key()));
        if (!key)
            return key;

        auto value = bb().compile_expr(TIRO_NN(entry->value()));
        if (!value)
            return value;

        pairs.append(*key);
        pairs.append(*value);
    }

    auto pairs_id = result().make(std::move(pairs));
    return bb().compile_rvalue(
        RValue::make_container(ContainerType::Map, pairs_id));
}

ExprResult ExprMIRGen::visit_null_literal([[maybe_unused]] NullLiteral* expr) {
    return bb().compile_rvalue(Constant::make_null());
}

ExprResult ExprMIRGen::visit_set_literal(SetLiteral* expr) {
    auto items = compile_exprs(TIRO_NN(expr->entries()));
    if (!items)
        return items.failure();

    return bb().compile_rvalue(
        RValue::make_container(ContainerType::Set, *items));
}

ExprResult ExprMIRGen::visit_string_literal(StringLiteral* expr) {
    TIRO_ASSERT(expr->value(), "Invalid string literal.");

    auto constant = Constant::make_string(expr->value());
    return bb().compile_rvalue(constant);
}

ExprResult ExprMIRGen::visit_symbol_literal(SymbolLiteral* expr) {
    TIRO_ASSERT(expr->value(), "Invalid symbol literal.");

    auto constant = Constant::make_symbol(expr->value());
    return bb().compile_rvalue(constant);
}

ExprResult ExprMIRGen::visit_tuple_literal(TupleLiteral* expr) {
    auto items = compile_exprs(TIRO_NN(expr->entries()));
    if (!items)
        return items.failure();

    return bb().compile_rvalue(
        RValue::make_container(ContainerType::Tuple, *items));
}

ExprResult ExprMIRGen::visit_return_expr(ReturnExpr* expr) {
    LocalID local;
    if (auto inner = expr->inner()) {
        auto result = dispatch(TIRO_NN(inner));
        if (!result)
            return result;
        local = *result;
    } else {
        local = bb().compile_rvalue(Constant::make_null());
    }

    bb().end(Terminator::make_return(local, result().exit()));
    return unreachable;
}

ExprResult ExprMIRGen::visit_string_sequence_expr(StringSequenceExpr* expr) {
    TIRO_ERROR("Invalid expression type in mir transform phase: {}.",
        to_string(expr->type()));
}

ExprResult ExprMIRGen::visit_tuple_member_expr(TupleMemberExpr* expr) {
    auto inner = bb().compile_expr(TIRO_NN(expr->inner()));
    if (!inner)
        return inner;

    auto lvalue = LValue::make_tuple_field(*inner, expr->index());
    return bb().compile_rvalue(RValue::make_use_lvalue(lvalue));
}

ExprResult ExprMIRGen::visit_unary_expr(UnaryExpr* expr) {
    auto op = map_operator(expr->operation());
    auto operand = bb().compile_expr(TIRO_NN(expr->inner()));
    if (!operand)
        return operand;

    return bb().compile_rvalue(RValue::make_unary_op(op, *operand));
}

ExprResult ExprMIRGen::visit_var_expr(VarExpr* expr) {
    auto symbol_ref = expr->resolved_symbol();
    TIRO_ASSERT(symbol_ref, "Variable was not resolved.");

    auto symbol = TIRO_NN(symbol_ref.get());
    return bb().compile_reference(symbol);
}

ExprResult ExprMIRGen::compile_assign(NotNull<Expr*> lhs, NotNull<Expr*> rhs) {

    AssignmentVisitor v{this, rhs};
    visit(lhs, v);
    return v.take_result();
}

enum class LogicalOp { And, Or };

ExprResult ExprMIRGen::compile_or(NotNull<Expr*> lhs, NotNull<Expr*> rhs) {
    return compile_logical_op(LogicalOp::Or, lhs, rhs);
}

ExprResult ExprMIRGen::compile_and(NotNull<Expr*> lhs, NotNull<Expr*> rhs) {
    return compile_logical_op(LogicalOp::And, lhs, rhs);
}

ExprResult ExprMIRGen::compile_logical_op(
    LogicalOp op, NotNull<Expr*> lhs, NotNull<Expr*> rhs) {
    const auto branch_name = strings().insert(
        op == LogicalOp::And ? "and-then" : "or-else");
    const auto end_name = strings().insert(
        op == LogicalOp::And ? "and-end" : "or-end");
    const auto branch_type = op == LogicalOp::And ? BranchType::IfFalse
                                                  : BranchType::IfTrue;

    const auto lhs_result = bb().compile_expr(lhs);
    if (!lhs_result)
        return lhs_result;

    // Branch off into another block to compute the alternative value if the test fails.
    // The resulting value is a phi node (unless values are trivially the same).
    const auto branch_block = ctx().make_block(branch_name);
    const auto end_block = ctx().make_block(end_name);
    bb().end(Terminator::make_branch(
        branch_type, *lhs_result, end_block, branch_block));
    ctx().seal(branch_block);

    const auto rhs_result = [&]() {
        auto nested = ctx().make_current(branch_block);
        auto result = nested.compile_expr(rhs);
        if (!result)
            return result;

        nested.end(Terminator::make_jump(end_block));
        return result;
    }();

    ctx().seal(end_block);
    bb().assign(end_block);

    // Avoid trivial phi nodes if the rhs is unreachable or both sides
    // evaluate to the same value.
    if (!rhs_result || *lhs_result == *rhs_result) {
        return *lhs_result;
    }

    auto phi_id = result().make(Phi({*lhs_result, *rhs_result}));
    return bb().compile_rvalue(RValue::make_phi(phi_id));
}

TransformResult<LocalListID>
ExprMIRGen::compile_exprs(NotNull<ExprList*> args) {
    LocalList local_args;
    for (const auto arg : args->entries()) {
        auto local = bb().compile_expr(TIRO_NN(arg));
        if (!local)
            return local.failure();

        local_args.append(*local);
    }

    return result().make(std::move(local_args));
}

BinaryOpType ExprMIRGen::map_operator(BinaryOperator op) {
    switch (op) {
#define TIRO_MAP(ast_op, mir_op) \
    case BinaryOperator::ast_op: \
        return BinaryOpType::mir_op;

        TIRO_MAP(Plus, Plus)
        TIRO_MAP(Minus, Minus)
        TIRO_MAP(Multiply, Multiply)
        TIRO_MAP(Divide, Divide)
        TIRO_MAP(Modulus, Modulus)
        TIRO_MAP(Power, Power)

        TIRO_MAP(LeftShift, LeftShift)
        TIRO_MAP(RightShift, RightShift)
        TIRO_MAP(BitwiseAnd, BitwiseAnd)
        TIRO_MAP(BitwiseOr, BitwiseOr)
        TIRO_MAP(BitwiseXor, BitwiseXor)

        TIRO_MAP(Less, Less)
        TIRO_MAP(LessEquals, LessEquals)
        TIRO_MAP(Greater, Greater)
        TIRO_MAP(GreaterEquals, GreaterEquals)
        TIRO_MAP(Equals, Equals)
        TIRO_MAP(NotEquals, NotEquals)
#undef TIRO_MAP

    case BinaryOperator::Assign:
    case BinaryOperator::AssignPlus:
    case BinaryOperator::AssignMinus:
    case BinaryOperator::AssignMultiply:
    case BinaryOperator::AssignDivide:
    case BinaryOperator::AssignModulus:
    case BinaryOperator::AssignPower:
    case BinaryOperator::LogicalAnd:
    case BinaryOperator::LogicalOr:
        TIRO_ERROR(
            "Binary operator in mir transformation phase should have been "
            "lowered: {}.",
            to_string(op));
    }

    TIRO_UNREACHABLE("Invalid binary operator.");
};

UnaryOpType ExprMIRGen::map_operator(UnaryOperator op) {
    switch (op) {
#define TIRO_MAP(ast_op, mir_op) \
    case UnaryOperator::ast_op:  \
        return UnaryOpType::mir_op;

        TIRO_MAP(Plus, Plus)
        TIRO_MAP(Minus, Minus);
        TIRO_MAP(BitwiseNot, BitwiseNot);
        TIRO_MAP(LogicalNot, LogicalNot);

#undef TIRO_MAP
    }

    TIRO_UNREACHABLE("Invalid unary operator.");
}

} // namespace tiro
