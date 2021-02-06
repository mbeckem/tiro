#include "compiler/ir_gen/compile.hpp"

#include "common/fix.hpp"
#include "compiler/ast/ast.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/ir_gen/func.hpp"
#include "compiler/ir_gen/module.hpp"
#include "compiler/semantics/symbol_table.hpp"
#include "compiler/semantics/type_table.hpp"

namespace tiro::ir {

namespace {

class ExprCompiler final : public Transformer {
public:
    ExprCompiler(FunctionIRGen& ctx, ExprOptions opts);

    InstResult dispatch(NotNull<AstExpr*> expr, CurrentBlock& bb);

    /* [[[cog
        from cog import outl
        from codegen.ast import NODE_TYPES, walk_concrete_types
        for node_type in walk_concrete_types(NODE_TYPES.get("Expr")):
            outl(f"InstResult {node_type.visitor_name}(NotNull<{node_type.cpp_name}*> expr, CurrentBlock& bb);")
    ]]] */
    InstResult visit_binary_expr(NotNull<AstBinaryExpr*> expr, CurrentBlock& bb);
    InstResult visit_block_expr(NotNull<AstBlockExpr*> expr, CurrentBlock& bb);
    InstResult visit_break_expr(NotNull<AstBreakExpr*> expr, CurrentBlock& bb);
    InstResult visit_call_expr(NotNull<AstCallExpr*> expr, CurrentBlock& bb);
    InstResult visit_continue_expr(NotNull<AstContinueExpr*> expr, CurrentBlock& bb);
    InstResult visit_element_expr(NotNull<AstElementExpr*> expr, CurrentBlock& bb);
    InstResult visit_func_expr(NotNull<AstFuncExpr*> expr, CurrentBlock& bb);
    InstResult visit_if_expr(NotNull<AstIfExpr*> expr, CurrentBlock& bb);
    InstResult visit_array_literal(NotNull<AstArrayLiteral*> expr, CurrentBlock& bb);
    InstResult visit_boolean_literal(NotNull<AstBooleanLiteral*> expr, CurrentBlock& bb);
    InstResult visit_float_literal(NotNull<AstFloatLiteral*> expr, CurrentBlock& bb);
    InstResult visit_integer_literal(NotNull<AstIntegerLiteral*> expr, CurrentBlock& bb);
    InstResult visit_map_literal(NotNull<AstMapLiteral*> expr, CurrentBlock& bb);
    InstResult visit_null_literal(NotNull<AstNullLiteral*> expr, CurrentBlock& bb);
    InstResult visit_record_literal(NotNull<AstRecordLiteral*> expr, CurrentBlock& bb);
    InstResult visit_set_literal(NotNull<AstSetLiteral*> expr, CurrentBlock& bb);
    InstResult visit_string_literal(NotNull<AstStringLiteral*> expr, CurrentBlock& bb);
    InstResult visit_symbol_literal(NotNull<AstSymbolLiteral*> expr, CurrentBlock& bb);
    InstResult visit_tuple_literal(NotNull<AstTupleLiteral*> expr, CurrentBlock& bb);
    InstResult visit_property_expr(NotNull<AstPropertyExpr*> expr, CurrentBlock& bb);
    InstResult visit_return_expr(NotNull<AstReturnExpr*> expr, CurrentBlock& bb);
    InstResult visit_string_expr(NotNull<AstStringExpr*> expr, CurrentBlock& bb);
    InstResult visit_string_group_expr(NotNull<AstStringGroupExpr*> expr, CurrentBlock& bb);
    InstResult visit_unary_expr(NotNull<AstUnaryExpr*> expr, CurrentBlock& bb);
    InstResult visit_var_expr(NotNull<AstVarExpr*> expr, CurrentBlock& bb);
    // [[[end]]]

private:
    struct ShortCircuitOp {
        // Name of the block for executing the branch-protected code.
        std::string_view branch_name;

        // Name of the block that merges the control flow again.
        std::string_view end_name;

        // Branch that implements the short circuit (e.g. IfFalse for 'And').
        BranchType branch_type;
    };

    // Compiles the simple binary operator, e.g. "a + b";
    InstResult
    compile_binary(BinaryOpType op, NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb);

    // Compiles a path of member, element or call expressions. Paths support optional chaining
    // with long short-circuiting. For example `a?.b.c.d` will not access `a.b.c.d` if `a` is null.
    InstResult compile_path(NotNull<AstExpr*> topmost, CurrentBlock& bb);

    // Compiles short-circuiting operators.
    InstResult compile_or(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb);
    InstResult compile_and(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb);
    InstResult compile_coalesce(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb);
    InstResult compile_short_circuit_op(
        const ShortCircuitOp& op, NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb);

    ExprType get_type(NotNull<AstExpr*> expr) const;
    SymbolId get_symbol(NotNull<AstVarExpr*> expr) const;

    bool can_elide() const;

private:
    ExprOptions opts_;
};

class PathCompiler final {
public:
    PathCompiler(FunctionIRGen& ctx, CurrentBlock& outer_bb);

    InstResult compile(NotNull<AstExpr*> topmost);

private:
    // Walks an expression path and handles optional value accesses if needed. This implements
    // the long short-circuiting behaviour of optional value accesses.
    InstResult compile_path(NotNull<AstExpr*> expr);

    InstResult compile_property(NotNull<AstPropertyExpr*> expr);
    InstResult compile_element(NotNull<AstElementExpr*> expr);
    InstResult compile_call(NotNull<AstCallExpr*> expr);

    // Assigns a new block to `chain_bb_` that is only entered when the value is not null.
    // Compilation continues in that new block.
    void enter_optional(std::string_view label, InstId value);

    // Lazily initializes the end block and returns its id.
    BlockId end_block();

    StringTable& strings() const { return ctx_.strings(); }
    Function& result() const { return ctx_.result(); }
    FunctionIRGen& ctx() const { return ctx_; }

    static bool is_path_element(NotNull<AstExpr*> expr) {
        return is_instance<AstPropertyExpr>(expr) || is_instance<AstElementExpr>(expr)
               || is_instance<AstCallExpr>(expr);
    }

    static bool is_method_call(NotNull<AstCallExpr*> expr) {
        auto func = TIRO_NN(expr->func());
        auto prop = try_cast<AstPropertyExpr>(func);
        // TODO: numeric members are not supported because the IR currently requires string names for method calls.
        return prop && is_instance<AstStringIdentifier>(prop->property());
    }

private:
    FunctionIRGen& ctx_;

    // The original block. This will be adjusted when compilation of the path is done.
    CurrentBlock& outer_bb_;

    // The current block while compiling the chain of element accesses. This may
    // be nested when optional values are encountered (e.g. `a?.b?.c` c will be compiled in the
    // basic block that is executed only when a and b are not null).
    CurrentBlock chain_bb_;

    // The end block is the jump target when either an optional value is null or when the chain has been
    // fully evaluated. The block is initialized lazily because it is only needed when an optional
    // path element is encountered, otherwise the compilation can simply proceed in the original block.
    std::optional<BlockId> end_block_;

    // Optional values that evaluate to null that have been encountered while compiling the path.
    LocalList::Storage optional_values_;
};

} // namespace

template<typename Range>
static bool all_equal(const Range& r) {
    auto pos = r.begin();
    auto end = r.end();
    TIRO_DEBUG_ASSERT(pos != end, "Range must not be empty.");

    auto first = *pos;
    ++pos;
    return std::all_of(pos, end, [&](const auto& e) { return first == e; });
}

template<typename T>
static TransformResult<LocalListId> compile_exprs(const AstNodeList<T>& args, CurrentBlock& bb) {
    LocalList local_args;
    for (auto arg : args) {
        auto inst_id = bb.compile_expr(TIRO_NN(arg));
        if (!inst_id)
            return inst_id.failure();

        local_args.append(*inst_id);
    }

    return bb.ctx().result().make(std::move(local_args));
}

PathCompiler::PathCompiler(FunctionIRGen& ctx, CurrentBlock& outer_bb)
    : ctx_(ctx)
    , outer_bb_(outer_bb)
    , chain_bb_(ctx_.make_current(outer_bb.id())) {}

InstResult PathCompiler::compile(NotNull<AstExpr*> topmost) {
    TIRO_DEBUG_ASSERT(is_path_element(topmost), "The topmost node must start a path.");

    auto chain_result = compile_path(topmost);
    if (chain_result)
        optional_values_.push_back(*chain_result);

    // If an end block was created due to optional accesses, continue in that block. Otherwise,
    // we must still be in the original block.
    if (end_block_) {
        chain_bb_.end(Terminator::make_jump(*end_block_));
        ctx().seal(*end_block_);
        chain_bb_.assign(*end_block_);
    }

    outer_bb_.assign(chain_bb_.id());

    if (optional_values_.empty())
        return chain_result; // Unreachable

    if (optional_values_.size() == 1 || all_equal(optional_values_))
        return optional_values_[0]; // Avoid unneccessary phi nodes

    auto phi_operands_id = result().make(LocalList(std::move(optional_values_)));
    return outer_bb_.compile_value(Phi(phi_operands_id));
}

InstResult PathCompiler::compile_path(NotNull<AstExpr*> expr) {
    if (!is_path_element(expr))
        return chain_bb_.compile_expr(expr);

    switch (expr->type()) {
    case AstNodeType::PropertyExpr:
        return compile_property(must_cast<AstPropertyExpr>(expr));
    case AstNodeType::ElementExpr:
        return compile_element(must_cast<AstElementExpr>(expr));
    case AstNodeType::CallExpr:
        return compile_call(must_cast<AstCallExpr>(expr));
    default:
        break;
    }

    TIRO_UNREACHABLE("Unhandled path element (invalid type).");
}

InstResult PathCompiler::compile_property(NotNull<AstPropertyExpr*> expr) {
    auto instance = compile_path(TIRO_NN(expr->instance()));
    if (!instance)
        return instance;

    switch (expr->access_type()) {
    case AccessType::Normal:
        break;
    case AccessType::Optional:
        enter_optional("instance-not-null", *instance);
        break;
    }

    auto lvalue = instance_field(*instance, TIRO_NN(expr->property()));
    return chain_bb_.compile_value(Value::make_read(lvalue));
}

InstResult PathCompiler::compile_element(NotNull<AstElementExpr*> expr) {
    auto instance = compile_path(TIRO_NN(expr->instance()));
    if (!instance)
        return instance;

    switch (expr->access_type()) {
    case AccessType::Normal:
        break;
    case AccessType::Optional:
        enter_optional("instance-not-null", *instance);
        break;
    }

    auto element = chain_bb_.compile_expr(TIRO_NN(expr->element()));
    if (!element)
        return element;

    auto lvalue = LValue::make_index(*instance, *element);
    return chain_bb_.compile_value(Value::make_read(lvalue));
}

InstResult PathCompiler::compile_call(NotNull<AstCallExpr*> expr) {
    auto call = must_cast<AstCallExpr>(expr);
    if (is_method_call(call)) {
        auto method = must_cast<AstPropertyExpr>(call->func());
        auto instance = compile_path(TIRO_NN(method->instance()));
        if (!instance)
            return instance;

        // Handle access type of the wrapped property access, e.g. `instance?.method()`.
        switch (method->access_type()) {
        case AccessType::Normal:
            break;
        case AccessType::Optional:
            enter_optional("instance-not-null", *instance);
            break;
        }

        auto name = must_cast<AstStringIdentifier>(method->property())->value();
        TIRO_DEBUG_ASSERT(name, "Invalid property name.");

        auto method_value = chain_bb_.compile_value(Aggregate::make_method(*instance, name));

        // Handle access type of the method call itself, e.g. `instance.function?()`.
        switch (call->access_type()) {
        case AccessType::Normal:
            break;
        case AccessType::Optional: {
            auto method_func = chain_bb_.compile_value(
                Value::make_get_aggregate_member(method_value, AggregateMember::MethodFunction));
            enter_optional("method-not-null", method_func);
            break;
        }
        }

        auto args = compile_exprs(call->args(), chain_bb_);
        if (!args)
            return args.failure();

        return chain_bb_.compile_value(Value::make_method_call(method_value, *args));
    } else {
        auto func = compile_path(TIRO_NN(call->func()));
        if (!func)
            return func;

        switch (call->access_type()) {
        case AccessType::Normal:
            break;
        case AccessType::Optional:
            enter_optional("func-not-null", *func);
            break;
        }

        auto args = compile_exprs(call->args(), chain_bb_);
        if (!args)
            return args.failure();

        return chain_bb_.compile_value(Value::make_call(*func, *args));
    }
}

void PathCompiler::enter_optional(std::string_view label, InstId value) {
    auto not_null_block = ctx().make_block(strings().insert(label));
    chain_bb_.end(Terminator::make_branch(BranchType::IfNull, value, end_block(), not_null_block));
    ctx().seal(not_null_block);

    optional_values_.push_back(value);
    chain_bb_.assign(not_null_block);
}

BlockId PathCompiler::end_block() {
    if (!end_block_)
        end_block_ = ctx().make_block(strings().insert("optional-path-end"));
    return *end_block_;
}

ExprCompiler::ExprCompiler(FunctionIRGen& ctx, ExprOptions opts)
    : Transformer(ctx)
    , opts_(opts) {}

InstResult ExprCompiler::dispatch(NotNull<AstExpr*> expr, CurrentBlock& bb) {
    TIRO_DEBUG_ASSERT(
        !expr->has_error(), "Nodes with errors must not reach the ir transformation stage.");
    return visit(expr, *this, bb);
}

InstResult ExprCompiler::visit_binary_expr(NotNull<AstBinaryExpr*> expr, CurrentBlock& bb) {
    auto lhs = TIRO_NN(expr->left());
    auto rhs = TIRO_NN(expr->right());

    switch (expr->operation()) {
#define TIRO_BINARY(AstOp, IROp) \
    case BinaryOperator::AstOp:  \
        return compile_binary(BinaryOpType::IROp, lhs, rhs, bb);

#define TIRO_ASSIGN_BINARY(AstOp, IROp) \
    case BinaryOperator::AstOp:         \
        return compile_compound_assign_expr(BinaryOpType::IROp, lhs, rhs, bb);

    case BinaryOperator::Assign:
        return compile_assign_expr(lhs, rhs, bb);
    case BinaryOperator::LogicalOr:
        return compile_or(lhs, rhs, bb);
    case BinaryOperator::LogicalAnd:
        return compile_and(lhs, rhs, bb);
    case BinaryOperator::NullCoalesce:
        return compile_coalesce(lhs, rhs, bb);

        TIRO_BINARY(Plus, Plus)
        TIRO_BINARY(Minus, Minus)
        TIRO_BINARY(Multiply, Multiply)
        TIRO_BINARY(Divide, Divide)
        TIRO_BINARY(Modulus, Modulus)
        TIRO_BINARY(Power, Power)

        TIRO_BINARY(LeftShift, LeftShift)
        TIRO_BINARY(RightShift, RightShift)
        TIRO_BINARY(BitwiseAnd, BitwiseAnd)
        TIRO_BINARY(BitwiseOr, BitwiseOr)
        TIRO_BINARY(BitwiseXor, BitwiseXor)

        TIRO_BINARY(Less, Less)
        TIRO_BINARY(LessEquals, LessEquals)
        TIRO_BINARY(Greater, Greater)
        TIRO_BINARY(GreaterEquals, GreaterEquals)
        TIRO_BINARY(Equals, Equals)
        TIRO_BINARY(NotEquals, NotEquals)

        TIRO_ASSIGN_BINARY(AssignPlus, Plus)
        TIRO_ASSIGN_BINARY(AssignMinus, Minus)
        TIRO_ASSIGN_BINARY(AssignMultiply, Multiply)
        TIRO_ASSIGN_BINARY(AssignDivide, Divide)
        TIRO_ASSIGN_BINARY(AssignModulus, Modulus)
        TIRO_ASSIGN_BINARY(AssignPower, Power)

#undef TIRO_BINARY
#undef TIRO_ASSIGN_BINARY
    }

    TIRO_UNREACHABLE("Invalid binary operation type.");
}

InstResult ExprCompiler::visit_block_expr(NotNull<AstBlockExpr*> expr, CurrentBlock& bb) {
    // Statements in this block expr can register deferred expressions.
    auto scope = ctx().enter_scope();
    auto scope_id = ctx().current_scope_id(); // TODO Awkward api

    auto& stmts = expr->stmts();

    bool has_value = can_use_as_value(get_type(expr));
    TIRO_CHECK(!has_value || stmts.size() > 0,
        "A block expression that produces a value must have at least one "
        "statement.");

    const size_t plain_stmts = stmts.size() - (has_value ? 1 : 0);
    for (size_t i = 0; i < plain_stmts; ++i) {
        auto result = bb.compile_stmt(TIRO_NN(stmts.get(i)));
        if (!result)
            return result.failure();
    }

    // Evaluate the return value expression (if any) before leaving the scope.
    auto result = [&]() -> InstResult {
        if (has_value) {
            auto last = try_cast<AstExprStmt>(stmts.get(plain_stmts));
            TIRO_CHECK(last,
                "The last statement must be an expression statement because "
                "this block produces a value.");

            return bb.compile_expr(TIRO_NN(last->expr()));
        }

        // Blocks without a value don't return a value. This would be safer
        // if we had a real type system.
        TIRO_DEBUG_ASSERT(can_elide(), "Must be able to elide value generation.");
        return InstId();
    }();

    // No need to generate scope exit code if we're unreachable anyway.
    if (!result)
        return result;

    // Evaluate deferred statements.
    TIRO_DEBUG_ASSERT(scope_id == ctx().current_scope_id(), "Must still be in the original scope.");
    auto exit_result = ctx().compile_scope_exit(scope_id, bb);
    if (!exit_result)
        return exit_result.failure();

    return result;
}

InstResult
ExprCompiler::visit_break_expr([[maybe_unused]] NotNull<AstBreakExpr*> expr, CurrentBlock& bb) {
    auto loop = ctx().current_loop();
    TIRO_CHECK(loop, "Break outside a loop.");

    auto exit_result = ctx().compile_scope_exit_until(ctx().current_loop_id(), bb);
    if (!exit_result)
        return exit_result.failure();

    auto target = loop->as_loop().jump_break;
    TIRO_DEBUG_ASSERT(target, "Current loop has an invalid break label.");
    bb.end(Terminator::make_jump(target));
    return unreachable;
}

InstResult ExprCompiler::visit_call_expr(NotNull<AstCallExpr*> expr, CurrentBlock& bb) {
    return compile_path(expr, bb);
}

InstResult ExprCompiler::visit_continue_expr(
    [[maybe_unused]] NotNull<AstContinueExpr*> expr, CurrentBlock& bb) {
    auto loop = ctx().current_loop();
    TIRO_CHECK(loop, "Continue outside a loop.");

    auto exit_result = ctx().compile_scope_exit_until(ctx().current_loop_id(), bb);
    if (!exit_result)
        return exit_result.failure();

    auto target = loop->as_loop().jump_continue;
    TIRO_DEBUG_ASSERT(target, "Current loop has an invalid continue label.");
    bb.end(Terminator::make_jump(target));
    return unreachable;
}

InstResult ExprCompiler::visit_element_expr(NotNull<AstElementExpr*> expr, CurrentBlock& bb) {
    return compile_path(expr, bb);
}

InstResult ExprCompiler::visit_func_expr(NotNull<AstFuncExpr*> expr, CurrentBlock& bb) {
    auto decl = TIRO_NN(expr->decl());
    auto envs = ctx().envs();
    auto env = ctx().current_env();

    ModuleMemberId func_id = ctx().module_gen().add_function(decl, envs, env);
    auto lvalue = LValue::make_module(func_id);
    auto func_inst = bb.compile_value(Value::make_read(lvalue));

    if (env) {
        auto env_id = bb.compile_env(env);
        return bb.compile_value(Value::make_make_closure(env_id, func_inst));
    }
    return func_inst;
}

InstResult ExprCompiler::visit_if_expr(NotNull<AstIfExpr*> expr, CurrentBlock& bb) {
    const auto type = get_type(expr);
    const bool has_value = can_use_as_value(type);

    const auto cond_result = bb.compile_expr(TIRO_NN(expr->cond()));
    if (!cond_result)
        return cond_result;

    if (!expr->else_branch()) {
        TIRO_DEBUG_ASSERT(!has_value, "If expr cannot have a value without an else-branch.");

        auto then_block = ctx().make_block(strings().insert("if-then"));
        auto end_block = ctx().make_block(strings().insert("if-end"));
        bb.end(Terminator::make_branch(BranchType::IfTrue, *cond_result, then_block, end_block));
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
        bb.assign(end_block);
        TIRO_DEBUG_ASSERT(can_elide(), "Must be able to elide value generation.");
        return InstId();
    }

    auto then_block = ctx().make_block(strings().insert("if-then"));
    auto else_block = ctx().make_block(strings().insert("if-else"));
    auto end_block = ctx().make_block(strings().insert("if-end"));
    bb.end(Terminator::make_branch(BranchType::IfTrue, *cond_result, then_block, else_block));
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
    bb.assign(end_block);

    if (!has_value) {
        TIRO_DEBUG_ASSERT(can_elide(), "Must be able to elide value generation.");
        return InstId();
    }
    if (!then_result)
        return else_result;
    if (!else_result)
        return then_result;

    // Avoid trivial phi nodes.
    if (*then_result == *else_result) {
        return *then_result;
    }

    return bb.compile_value(Phi(result(), {*then_result, *else_result}));
}

InstResult ExprCompiler::visit_array_literal(NotNull<AstArrayLiteral*> expr, CurrentBlock& bb) {
    auto items = compile_exprs(expr->items(), bb);
    if (!items)
        return items.failure();

    return bb.compile_value(Value::make_container(ContainerType::Array, *items));
}

InstResult ExprCompiler::visit_boolean_literal(NotNull<AstBooleanLiteral*> expr, CurrentBlock& bb) {
    auto value = expr->value() ? Constant::make_true() : Constant::make_false();
    return bb.compile_value(value);
}

InstResult ExprCompiler::visit_float_literal(NotNull<AstFloatLiteral*> expr, CurrentBlock& bb) {
    auto constant = Constant::make_float(expr->value());
    return bb.compile_value(constant);
}

InstResult ExprCompiler::visit_integer_literal(NotNull<AstIntegerLiteral*> expr, CurrentBlock& bb) {
    auto constant = Constant::make_integer(expr->value());
    return bb.compile_value(constant);
}

InstResult ExprCompiler::visit_map_literal(NotNull<AstMapLiteral*> expr, CurrentBlock& bb) {
    LocalList pairs;
    for (const auto entry : expr->items()) {
        auto key = bb.compile_expr(TIRO_NN(entry->key()));
        if (!key)
            return key;

        auto value = bb.compile_expr(TIRO_NN(entry->value()));
        if (!value)
            return value;

        pairs.append(*key);
        pairs.append(*value);
    }

    auto pairs_id = result().make(std::move(pairs));
    return bb.compile_value(Value::make_container(ContainerType::Map, pairs_id));
}

InstResult ExprCompiler::visit_record_literal(NotNull<AstRecordLiteral*> expr, CurrentBlock& bb) {
    Record record;
    for (const auto entry : expr->items()) {
        auto key = TIRO_NN(entry->key())->value();
        auto value = bb.compile_expr(TIRO_NN(entry->value()));
        if (!value)
            return value;

        record.insert(key, *value);
    }

    auto record_id = result().make(std::move(record));
    return bb.compile_value(Value::make_record(record_id));
}

InstResult
ExprCompiler::visit_null_literal([[maybe_unused]] NotNull<AstNullLiteral*> expr, CurrentBlock& bb) {
    return bb.compile_value(Constant::make_null());
}

InstResult ExprCompiler::visit_set_literal(NotNull<AstSetLiteral*> expr, CurrentBlock& bb) {
    auto items = compile_exprs(expr->items(), bb);
    if (!items)
        return items.failure();

    return bb.compile_value(Value::make_container(ContainerType::Set, *items));
}

InstResult ExprCompiler::visit_string_literal(NotNull<AstStringLiteral*> expr, CurrentBlock& bb) {
    TIRO_DEBUG_ASSERT(expr->value(), "Invalid string literal.");

    auto constant = Constant::make_string(expr->value());
    return bb.compile_value(constant);
}

InstResult ExprCompiler::visit_symbol_literal(NotNull<AstSymbolLiteral*> expr, CurrentBlock& bb) {
    TIRO_DEBUG_ASSERT(expr->value(), "Invalid symbol literal.");

    auto constant = Constant::make_symbol(expr->value());
    return bb.compile_value(constant);
}

InstResult ExprCompiler::visit_tuple_literal(NotNull<AstTupleLiteral*> expr, CurrentBlock& bb) {
    auto items = compile_exprs(expr->items(), bb);
    if (!items)
        return items.failure();

    return bb.compile_value(Value::make_container(ContainerType::Tuple, *items));
}

InstResult ExprCompiler::visit_property_expr(NotNull<AstPropertyExpr*> expr, CurrentBlock& bb) {
    return compile_path(expr, bb);
}

InstResult ExprCompiler::visit_return_expr(NotNull<AstReturnExpr*> expr, CurrentBlock& bb) {
    InstId inst;
    if (auto value = expr->value()) {
        auto result = dispatch(TIRO_NN(value), bb);
        if (!result)
            return result;
        inst = *result;
    } else {
        inst = bb.compile_value(Constant::make_null());
    }

    auto exit_result = ctx().compile_scope_exit_until(RegionId(), bb);
    if (!exit_result)
        return exit_result.failure();

    bb.end(Terminator::make_return(inst, result().exit()));
    return unreachable;
}

InstResult ExprCompiler::visit_string_expr(NotNull<AstStringExpr*> expr, CurrentBlock& bb) {
    const auto items = compile_exprs(expr->items(), bb);
    if (!items)
        return items.failure();

    return bb.compile_value(Value::make_format(*items));
}

InstResult
ExprCompiler::visit_string_group_expr(NotNull<AstStringGroupExpr*> expr, CurrentBlock& bb) {
    const auto items = compile_exprs(expr->strings(), bb);
    if (!items)
        return items.failure();

    return bb.compile_value(Value::make_format(*items));
}

InstResult ExprCompiler::visit_unary_expr(NotNull<AstUnaryExpr*> expr, CurrentBlock& bb) {
    auto op = [&] {
        switch (expr->operation()) {
#define TIRO_MAP(AstOp, IROp)  \
    case UnaryOperator::AstOp: \
        return UnaryOpType::IROp;

            TIRO_MAP(Plus, Plus)
            TIRO_MAP(Minus, Minus);
            TIRO_MAP(BitwiseNot, BitwiseNot);
            TIRO_MAP(LogicalNot, LogicalNot);

#undef TIRO_MAP
        }
        TIRO_UNREACHABLE("Invalid unary operator.");
    }();

    auto operand = bb.compile_expr(TIRO_NN(expr->inner()));
    if (!operand)
        return operand;

    return bb.compile_value(Value::make_unary_op(op, *operand));
}

InstResult ExprCompiler::visit_var_expr(NotNull<AstVarExpr*> expr, CurrentBlock& bb) {
    auto symbol = get_symbol(expr);
    return bb.compile_read(symbol);
}

InstResult ExprCompiler::compile_binary(
    BinaryOpType op, NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb) {
    auto lhs_value = bb.compile_expr(lhs);
    if (!lhs_value)
        return lhs_value;

    auto rhs_value = bb.compile_expr(rhs);
    if (!rhs_value)
        return rhs_value;

    return bb.compile_value(Value::make_binary_op(op, *lhs_value, *rhs_value));
}

InstResult ExprCompiler::compile_path(NotNull<AstExpr*> topmost, CurrentBlock& bb) {
    PathCompiler path(ctx(), bb);
    return path.compile(topmost);
}

InstResult
ExprCompiler::compile_or(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb) {
    static constexpr ShortCircuitOp op{
        "or-else",
        "or-end",
        BranchType::IfTrue,
    };
    return compile_short_circuit_op(op, lhs, rhs, bb);
}

InstResult
ExprCompiler::compile_and(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb) {
    static constexpr ShortCircuitOp op{
        "and-then",
        "and-end",
        BranchType::IfFalse,
    };
    return compile_short_circuit_op(op, lhs, rhs, bb);
}

InstResult
ExprCompiler::compile_coalesce(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb) {
    static constexpr ShortCircuitOp op{
        "null-else",
        "null-end",
        BranchType::IfNotNull,
    };
    return compile_short_circuit_op(op, lhs, rhs, bb);
}

InstResult ExprCompiler::compile_short_circuit_op(
    const ShortCircuitOp& op, NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb) {
    const auto lhs_result = bb.compile_expr(lhs);
    if (!lhs_result)
        return lhs_result;

    // Branch off into another block to compute the alternative value if the test fails.
    // The resulting value is a phi node (unless values are trivially the same).
    const auto branch_block = ctx().make_block(strings().insert(op.branch_name));
    const auto end_block = ctx().make_block(strings().insert(op.end_name));
    bb.end(Terminator::make_branch(op.branch_type, *lhs_result, end_block, branch_block));
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
    bb.assign(end_block);

    // Avoid trivial phi nodes if the rhs is unreachable or both sides
    // evaluate to the same value.
    if (!rhs_result || *lhs_result == *rhs_result)
        return *lhs_result;

    return bb.compile_value(Phi(result(), {*lhs_result, *rhs_result}));
}

ExprType ExprCompiler::get_type(NotNull<AstExpr*> expr) const {
    return types().get_type(expr->id());
}

SymbolId ExprCompiler::get_symbol(NotNull<AstVarExpr*> expr) const {
    return symbols().get_ref(expr->id());
}

[[maybe_unused]] bool ExprCompiler::can_elide() const {
    return has_options(opts_, ExprOptions::MaybeInvalid);
}

LValue instance_field(InstId instance, NotNull<AstIdentifier*> identifier) {
    struct InstanceFieldVisitor {
        InstId instance;

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

InstResult compile_expr(NotNull<AstExpr*> expr, CurrentBlock& bb) {
    return compile_expr(expr, ExprOptions::Default, bb);
}

InstResult compile_expr(NotNull<AstExpr*> expr, ExprOptions options, CurrentBlock& bb) {
    ExprCompiler gen(bb.ctx(), options);

    auto result = gen.dispatch(expr, bb);
    if (result && !has_options(options, ExprOptions::MaybeInvalid)) {
        TIRO_DEBUG_ASSERT(result.value().valid(),
            "Expression transformation must return a valid instruction in this "
            "context.");
    }

    return result;
}

} // namespace tiro::ir
