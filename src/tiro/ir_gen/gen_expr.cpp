#include "tiro/ir_gen/gen_expr.hpp"

#include "tiro/ast/ast.hpp"
#include "tiro/ir/function.hpp"
#include "tiro/ir_gen/gen_func.hpp"
#include "tiro/ir_gen/gen_module.hpp"
#include "tiro/semantics/symbol_table.hpp"
#include "tiro/semantics/type_table.hpp"

namespace tiro {

// TODO: Move down, do not implement assignment visitor functions inline.
LValue instance_field(LocalId instance, NotNull<AstIdentifier*> identifier) {
    struct InstanceFieldVisitor {
        LocalId instance;

        LValue visit_numeric_identifier(NotNull<AstNumericIdentifier*> field) {
            return LValue::make_tuple_field(instance, field->value());
        }

        LValue visit_string_identifier(NotNull<AstStringIdentifier*> field) {
            TIRO_DEBUG_ASSERT(field->value().valid(), "Invalid field name.");
            return LValue::make_field(instance, field->value());
        }
    };

    return visit(identifier, InstanceFieldVisitor{instance});
}

namespace {

class AssignmentVisitor : public DefaultNodeVisitor<AssignmentVisitor> {
public:
    explicit AssignmentVisitor(ExprIRGen& expr_gen, NotNull<AstExpr*> rhs)
        : expr_gen_(expr_gen)
        , rhs_(rhs)
        , result_(unreachable) {}

    LocalResult&& take_result() { return std::move(result_); }

    void visit_property_expr(NotNull<AstPropertyExpr*> lhs) TIRO_NODE_VISITOR_OVERRIDE {
        simple_assign(lhs);
    }

    void visit_element_expr(NotNull<AstElementExpr*> lhs) TIRO_NODE_VISITOR_OVERRIDE {
        simple_assign(lhs);
    }

    void visit_var_expr(NotNull<AstVarExpr*> lhs) TIRO_NODE_VISITOR_OVERRIDE { simple_assign(lhs); }

    void visit_tuple_literal(NotNull<AstTupleLiteral*> lhs) TIRO_NODE_VISITOR_OVERRIDE {
        const u32 target_count = lhs->items().size();

        // Left to right evaluation order.
        // TODO: Small vec.
        std::vector<AssignTarget> targets;
        targets.reserve(target_count);
        for (u32 i = 0; i < target_count; ++i) {
            auto expr = TIRO_NN(lhs->items().get(i));
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

    void visit_expr(NotNull<AstExpr*> lhs) TIRO_NODE_VISITOR_OVERRIDE {
        TIRO_ERROR("Invalid left hand side of type {} in assignment.", to_string(lhs->type()));
    }

private:
    template<typename T>
    void simple_assign(NotNull<T*> lhs) {
        auto target = target_for(lhs);
        if (!target)
            return complete(target.failure());

        auto rhs_result = compile_rhs();
        if (!rhs_result)
            return complete(rhs_result);

        bb().compile_assign(*target, *rhs_result);
        return complete(rhs_result);
    }

    TransformResult<AssignTarget> tuple_target_for(NotNull<AstExpr*> node) {
        struct Visitor : DefaultNodeVisitor<Visitor> {
            AssignmentVisitor* assign;
            TransformResult<AssignTarget> result = unreachable;

            Visitor(AssignmentVisitor* assign_)
                : assign(assign_) {}

            void visit_property_expr(NotNull<AstPropertyExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE {
                result = assign->target_for(expr);
            }

            void visit_element_expr(NotNull<AstElementExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE {
                result = assign->target_for(expr);
            }

            void visit_var_expr(NotNull<AstVarExpr*> expr) TIRO_NODE_VISITOR_OVERRIDE {
                result = assign->target_for(expr);
            }

            void visit_node(NotNull<AstNode*> node) TIRO_NODE_VISITOR_OVERRIDE {
                TIRO_ERROR("Invalid left hand side of type {} in tuple assignment.",
                    to_string(node->type()));
            }
        };

        Visitor visitor{this};
        visit(node, visitor);
        return visitor.result;
    }

    TransformResult<AssignTarget> target_for(NotNull<AstPropertyExpr*> expr) {
        auto instance_result = bb().compile_expr(TIRO_NN(expr->instance()));
        if (!instance_result)
            return instance_result.failure();

        auto lvalue = instance_field(*instance_result, TIRO_NN(expr->property()));
        return AssignTarget::make_lvalue(lvalue);
    }

    TransformResult<AssignTarget> target_for(NotNull<AstElementExpr*> expr) {
        auto array_result = bb().compile_expr(TIRO_NN(expr->instance()));
        if (!array_result)
            return array_result.failure();

        auto element_result = bb().compile_expr(TIRO_NN(expr->element()));
        if (!element_result)
            return element_result.failure();

        auto lvalue = LValue::make_index(*array_result, *element_result);
        return AssignTarget::make_lvalue(lvalue);
    }

    TransformResult<AssignTarget> target_for(NotNull<AstVarExpr*> expr) {
        auto symbol_id = symbols().get_ref(expr->id());
        return AssignTarget::make_symbol(symbol_id);
    }

    void complete(LocalResult result) { result_ = std::move(result); }

    LocalResult compile_rhs() { return bb().compile_expr(rhs_); }

    const SymbolTable& symbols() const { return expr_gen_.ctx().symbols(); }
    CurrentBlock& bb() const { return expr_gen_.bb(); }

private:
    ExprIRGen& expr_gen_;
    NotNull<AstExpr*> rhs_;
    LocalResult result_;
};

} // namespace

ExprIRGen::ExprIRGen(FunctionIRGen& ctx, ExprOptions opts, CurrentBlock& bb)
    : Transformer(ctx, bb)
    , opts_(opts) {}

LocalResult ExprIRGen::dispatch(NotNull<AstExpr*> expr) {
    TIRO_DEBUG_ASSERT(
        !expr->has_error(), "Nodes with errors must not reach the ir transformation stage.");
    return visit(expr, *this);
}

LocalResult ExprIRGen::visit_binary_expr(NotNull<AstBinaryExpr*> expr) {
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

LocalResult ExprIRGen::visit_block_expr(NotNull<AstBlockExpr*> expr) {
    auto& stmts = expr->stmts();

    bool has_value = can_use_as_value(get_type(expr));
    TIRO_CHECK(!has_value || stmts.size() > 0,
        "A block expression that produces a value must have at least one "
        "statement.");

    const size_t plain_stmts = stmts.size() - (has_value ? 1 : 0);
    for (size_t i = 0; i < plain_stmts; ++i) {
        auto result = bb().compile_stmt(TIRO_NN(stmts.get(i)));
        if (!result)
            return result.failure();
    }

    if (has_value) {
        auto last = try_cast<AstExprStmt>(stmts.get(plain_stmts));
        TIRO_CHECK(last,
            "The last statement must be an expression statement because "
            "this block produces a value.");

        return bb().compile_expr(TIRO_NN(last->expr()));
    }

    // Blocks without a value don't return a local. This would be safer
    // if we had a real type system.
    TIRO_DEBUG_ASSERT(can_elide(), "Must be able to elide value generation.");
    return LocalId();
}

LocalResult ExprIRGen::visit_break_expr([[maybe_unused]] NotNull<AstBreakExpr*> expr) {
    auto loop = current_loop();
    TIRO_CHECK(loop, "Break outside a loop.");

    auto target = loop->jump_break;
    TIRO_DEBUG_ASSERT(target, "Current loop has an invalid break label.");
    bb().end(Terminator::make_jump(target));
    return unreachable;
}

LocalResult ExprIRGen::visit_call_expr(NotNull<AstCallExpr*> expr) {
    const auto func = TIRO_NN(expr->func());

    // This is a member function invocation, i.e. a.b(...).
    if (auto prop = try_cast<AstPropertyExpr>(func);
        prop && is_instance<AstStringIdentifier>(prop->property())) {

        auto instance = bb().compile_expr(TIRO_NN(prop->instance()));
        if (!instance)
            return instance;

        auto name = static_cast<AstStringIdentifier*>(prop->property())->value();
        TIRO_DEBUG_ASSERT(name, "Invalid property name.");

        auto method = bb().compile_rvalue(RValue::make_method_handle(*instance, name));

        auto args = compile_exprs(expr->args());
        if (!args)
            return args.failure();
        return bb().compile_rvalue(RValue::make_method_call(method, *args));
    }

    // Otherwise: plain old function call.
    auto func_local = bb().compile_expr(func);
    if (!func_local)
        return func_local;

    auto args = compile_exprs(expr->args());
    if (!args)
        return args.failure();

    return bb().compile_rvalue(RValue::make_call(*func_local, *args));
}

LocalResult ExprIRGen::visit_continue_expr([[maybe_unused]] NotNull<AstContinueExpr*> expr) {
    auto loop = current_loop();
    TIRO_CHECK(loop, "Continue outside a loop.");

    auto target = loop->jump_continue;
    TIRO_DEBUG_ASSERT(target, "Current loop has an invalid break label.");
    bb().end(Terminator::make_jump(target));
    return unreachable;
}

LocalResult ExprIRGen::visit_element_expr(NotNull<AstElementExpr*> expr) {
    auto inner = bb().compile_expr(TIRO_NN(expr->instance()));
    if (!inner)
        return inner;

    auto element = bb().compile_expr(TIRO_NN(expr->element()));
    if (!element)
        return element;

    auto lvalue = LValue::make_index(*inner, *element);
    return bb().compile_rvalue(RValue::make_use_lvalue(lvalue));
}

LocalResult ExprIRGen::visit_func_expr(NotNull<AstFuncExpr*> expr) {
    auto decl = TIRO_NN(expr->decl());
    auto envs = ctx().envs();
    auto env = ctx().current_env();

    ModuleMemberId func_id = ctx().module_gen().add_function(decl, envs, env);
    auto lvalue = LValue::make_module(func_id);
    auto func_local = bb().compile_rvalue(RValue::make_use_lvalue(lvalue));

    if (env) {
        auto env_id = bb().compile_env(env);
        return bb().compile_rvalue(RValue::make_make_closure(env_id, func_local));
    }
    return func_local;
}

LocalResult ExprIRGen::visit_if_expr(NotNull<AstIfExpr*> expr) {
    const auto type = get_type(expr);
    const bool has_value = can_use_as_value(type);

    const auto cond_result = bb().compile_expr(TIRO_NN(expr->cond()));
    if (!cond_result)
        return cond_result;

    if (!expr->else_branch()) {
        TIRO_DEBUG_ASSERT(!has_value, "If expr cannot have a value without an else-branch.");

        auto then_block = ctx().make_block(strings().insert("if-then"));
        auto end_block = ctx().make_block(strings().insert("if-end"));
        bb().end(Terminator::make_branch(BranchType::IfTrue, *cond_result, then_block, end_block));
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
        TIRO_DEBUG_ASSERT(can_elide(), "Must be able to elide value generation.");
        return LocalId();
    }

    auto then_block = ctx().make_block(strings().insert("if-then"));
    auto else_block = ctx().make_block(strings().insert("if-else"));
    auto end_block = ctx().make_block(strings().insert("if-end"));
    bb().end(Terminator::make_branch(BranchType::IfTrue, *cond_result, then_block, else_block));
    ctx().seal(then_block);
    ctx().seal(else_block);

    const auto expr_options = has_value ? ExprOptions::Default : ExprOptions::MaybeInvalid;
    auto compile_branch = [&](BlockId block, NotNull<AstExpr*> branch) {
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

    if (!has_value) {
        TIRO_DEBUG_ASSERT(can_elide(), "Must be able to elide value generation.");
        return LocalId();
    }
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

LocalResult ExprIRGen::visit_array_literal(NotNull<AstArrayLiteral*> expr) {
    auto items = compile_exprs(expr->items());
    if (!items)
        return items.failure();

    return bb().compile_rvalue(RValue::make_container(ContainerType::Array, *items));
}

LocalResult ExprIRGen::visit_boolean_literal(NotNull<AstBooleanLiteral*> expr) {
    auto value = expr->value() ? Constant::make_true() : Constant::make_false();
    return bb().compile_rvalue(value);
}

LocalResult ExprIRGen::visit_float_literal(NotNull<AstFloatLiteral*> expr) {
    auto constant = Constant::make_float(expr->value());
    return bb().compile_rvalue(constant);
}

LocalResult ExprIRGen::visit_integer_literal(NotNull<AstIntegerLiteral*> expr) {
    auto constant = Constant::make_integer(expr->value());
    return bb().compile_rvalue(constant);
}

LocalResult ExprIRGen::visit_map_literal(NotNull<AstMapLiteral*> expr) {
    LocalList pairs;
    for (const auto entry : expr->items()) {
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
    return bb().compile_rvalue(RValue::make_container(ContainerType::Map, pairs_id));
}

LocalResult ExprIRGen::visit_null_literal([[maybe_unused]] NotNull<AstNullLiteral*> expr) {
    return bb().compile_rvalue(Constant::make_null());
}

LocalResult ExprIRGen::visit_set_literal(NotNull<AstSetLiteral*> expr) {
    auto items = compile_exprs(expr->items());
    if (!items)
        return items.failure();

    return bb().compile_rvalue(RValue::make_container(ContainerType::Set, *items));
}

LocalResult ExprIRGen::visit_string_literal(NotNull<AstStringLiteral*> expr) {
    TIRO_DEBUG_ASSERT(expr->value(), "Invalid string literal.");

    auto constant = Constant::make_string(expr->value());
    return bb().compile_rvalue(constant);
}

LocalResult ExprIRGen::visit_symbol_literal(NotNull<AstSymbolLiteral*> expr) {
    TIRO_DEBUG_ASSERT(expr->value(), "Invalid symbol literal.");

    auto constant = Constant::make_symbol(expr->value());
    return bb().compile_rvalue(constant);
}

LocalResult ExprIRGen::visit_tuple_literal(NotNull<AstTupleLiteral*> expr) {
    auto items = compile_exprs(expr->items());
    if (!items)
        return items.failure();

    return bb().compile_rvalue(RValue::make_container(ContainerType::Tuple, *items));
}

LocalResult ExprIRGen::visit_property_expr(NotNull<AstPropertyExpr*> expr) {
    auto instance = bb().compile_expr(TIRO_NN(expr->instance()));
    if (!instance)
        return instance;

    auto lvalue = instance_field(*instance, TIRO_NN(expr->property()));
    return bb().compile_rvalue(RValue::make_use_lvalue(lvalue));
}

LocalResult ExprIRGen::visit_return_expr(NotNull<AstReturnExpr*> expr) {
    LocalId local;
    if (auto value = expr->value()) {
        auto result = dispatch(TIRO_NN(value));
        if (!result)
            return result;
        local = *result;
    } else {
        local = bb().compile_rvalue(Constant::make_null());
    }

    bb().end(Terminator::make_return(local, result().exit()));
    return unreachable;
}

LocalResult ExprIRGen::visit_string_expr(NotNull<AstStringExpr*> expr) {
    const auto items = compile_exprs(expr->items());
    if (!items)
        return items.failure();

    return bb().compile_rvalue(RValue::make_format(*items));
}

LocalResult ExprIRGen::visit_string_group_expr(NotNull<AstStringGroupExpr*> expr) {
    // TODO: Need a test to ensure that compile time string merging works for constructs such as
    // const x = "a" "b" "c" // == "abc"
    TIRO_ERROR("Invalid expression type in ir transform phase: {}.", to_string(expr->type()));

    const auto items = compile_exprs(expr->strings());
    if (!items)
        return items.failure();

    return bb().compile_rvalue(RValue::make_format(*items));
}

LocalResult ExprIRGen::visit_unary_expr(NotNull<AstUnaryExpr*> expr) {
    auto op = map_operator(expr->operation());
    auto operand = bb().compile_expr(TIRO_NN(expr->inner()));
    if (!operand)
        return operand;

    return bb().compile_rvalue(RValue::make_unary_op(op, *operand));
}

LocalResult ExprIRGen::visit_var_expr(NotNull<AstVarExpr*> expr) {
    auto symbol = get_symbol(expr);
    return bb().compile_reference(symbol);
}

LocalResult ExprIRGen::compile_assign(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs) {
    AssignmentVisitor assign(*this, rhs);
    visit(lhs, assign);
    return assign.take_result();
}

LocalResult ExprIRGen::compile_or(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs) {
    return compile_logical_op(LogicalOp::Or, lhs, rhs);
}

LocalResult ExprIRGen::compile_and(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs) {
    return compile_logical_op(LogicalOp::And, lhs, rhs);
}

LocalResult
ExprIRGen::compile_logical_op(LogicalOp op, NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs) {
    const auto branch_name = strings().insert(op == LogicalOp::And ? "and-then" : "or-else");
    const auto end_name = strings().insert(op == LogicalOp::And ? "and-end" : "or-end");
    const auto branch_type = op == LogicalOp::And ? BranchType::IfFalse : BranchType::IfTrue;

    const auto lhs_result = bb().compile_expr(lhs);
    if (!lhs_result)
        return lhs_result;

    // Branch off into another block to compute the alternative value if the test fails.
    // The resulting value is a phi node (unless values are trivially the same).
    const auto branch_block = ctx().make_block(branch_name);
    const auto end_block = ctx().make_block(end_name);
    bb().end(Terminator::make_branch(branch_type, *lhs_result, end_block, branch_block));
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

template<typename ExprType>
TransformResult<LocalListId> ExprIRGen::compile_exprs(const AstNodeList<ExprType>& args) {
    LocalList local_args;
    for (auto arg : args) {
        auto local = bb().compile_expr(TIRO_NN(arg));
        if (!local)
            return local.failure();

        local_args.append(*local);
    }

    return result().make(std::move(local_args));
}

BinaryOpType ExprIRGen::map_operator(BinaryOperator op) {
    switch (op) {
#define TIRO_MAP(ast_op, ir_op)  \
    case BinaryOperator::ast_op: \
        return BinaryOpType::ir_op;

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
            "Binary operator in ir transformation phase should have been "
            "lowered: {}.",
            to_string(op));
    }

    TIRO_UNREACHABLE("Invalid binary operator.");
};

UnaryOpType ExprIRGen::map_operator(UnaryOperator op) {
    switch (op) {
#define TIRO_MAP(ast_op, ir_op) \
    case UnaryOperator::ast_op: \
        return UnaryOpType::ir_op;

        TIRO_MAP(Plus, Plus)
        TIRO_MAP(Minus, Minus);
        TIRO_MAP(BitwiseNot, BitwiseNot);
        TIRO_MAP(LogicalNot, LogicalNot);

#undef TIRO_MAP
    }

    TIRO_UNREACHABLE("Invalid unary operator.");
}

ValueType ExprIRGen::get_type(NotNull<AstExpr*> expr) const {
    return ctx().types().get_type(expr->id());
}

SymbolId ExprIRGen::get_symbol(NotNull<AstVarExpr*> expr) const {
    return ctx().symbols().get_ref(expr->id());
}

bool ExprIRGen::can_elide() const {
    return has_options(opts_, ExprOptions::MaybeInvalid);
}

} // namespace tiro
