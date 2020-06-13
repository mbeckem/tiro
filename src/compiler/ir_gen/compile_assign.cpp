#include "compiler/ir_gen/compile.hpp"

#include "common/math.hpp"
#include "compiler/ast/ast.hpp"
#include "compiler/semantics/symbol_table.hpp"

namespace tiro {

namespace {

class TargetVisitor final
    : public DefaultNodeVisitor<TargetVisitor, TransformResult<AssignTarget>&> {
public:
    explicit TargetVisitor(CurrentBlock& bb)
        : symbols_(bb.ctx().symbols())
        , bb_(bb) {}

    TransformResult<AssignTarget> run(NotNull<AstExpr*> expr) {
        TransformResult<AssignTarget> target = unreachable;
        visit(expr, *this, target);
        return target;
    }

    void visit_property_expr(NotNull<AstPropertyExpr*> expr,
        TransformResult<AssignTarget>& target) TIRO_NODE_VISITOR_OVERRIDE {
        target = target_for(expr);
    }

    void visit_element_expr(NotNull<AstElementExpr*> expr,
        TransformResult<AssignTarget>& target) TIRO_NODE_VISITOR_OVERRIDE {
        target = target_for(expr);
    }

    void visit_var_expr(NotNull<AstVarExpr*> expr,
        TransformResult<AssignTarget>& target) TIRO_NODE_VISITOR_OVERRIDE {
        target = target_for(expr);
    }

    void visit_expr(NotNull<AstExpr*> expr,
        [[maybe_unused]] TransformResult<AssignTarget>& target) TIRO_NODE_VISITOR_OVERRIDE {
        TIRO_ERROR("Invalid left hand side of type {} in assignment.", to_string(expr->type()));
    }

private:
    TransformResult<AssignTarget> target_for(NotNull<AstVarExpr*> expr);
    TransformResult<AssignTarget> target_for(NotNull<AstPropertyExpr*> expr);
    TransformResult<AssignTarget> target_for(NotNull<AstElementExpr*> expr);

private:
    const SymbolTable& symbols_;
    CurrentBlock& bb_;
};

struct BindingVisitor {
public:
    explicit BindingVisitor(CurrentBlock& bb)
        : bb_(bb) {}

    OkResult visit_var_binding(NotNull<AstVarBinding*> b) {
        auto symbol_id = symbols().get_decl(SymbolKey::for_node(b->id()));

        if (const auto& init = b->init()) {
            auto value = bb_.compile_expr(TIRO_NN(init));
            if (!value)
                return value.failure();

            bb_.compile_assign(symbol_id, *value);
        }
        return ok;
    }

    // TODO: If the initializer is a tuple literal (i.e. known contents at compile time)
    // we can skip generating the complete tuple and assign the individual variables directly.
    // We could also implement tuple construction at compilation time (const_eval.cpp) to optimize
    // this after the fact.
    OkResult visit_tuple_binding(NotNull<AstTupleBinding*> b) {
        if (const auto init = b->init()) {
            auto rhs = bb_.compile_expr(TIRO_NN(init));
            if (!rhs)
                return rhs.failure();

            const u32 var_count = checked_cast<u32>(b->names().size());
            if (var_count == 0)
                return ok;

            for (u32 i = 0; i < var_count; ++i) {
                auto symbol = symbols().get_decl(SymbolKey::for_element(b->id(), i));

                auto element = bb_.compile_rvalue(
                    RValue::UseLValue{LValue::make_tuple_field(*rhs, i)});
                bb_.compile_assign(symbol, element);
            }
        }
        return ok;
    }

private:
    const SymbolTable& symbols() const { return bb_.ctx().symbols(); }

private:
    CurrentBlock& bb_;
};

} // namespace

TransformResult<AssignTarget> TargetVisitor::target_for(NotNull<AstVarExpr*> expr) {
    auto symbol_id = symbols_.get_ref(expr->id());
    return AssignTarget::make_symbol(symbol_id);
}

TransformResult<AssignTarget> TargetVisitor::target_for(NotNull<AstPropertyExpr*> expr) {
    TIRO_DEBUG_ASSERT(expr->access_type() == AccessType::Normal,
        "Cannot use optional chaining expressions as the left hand side to an assignment.");

    auto instance_result = bb_.compile_expr(TIRO_NN(expr->instance()));
    if (!instance_result)
        return instance_result.failure();

    auto lvalue = instance_field(*instance_result, TIRO_NN(expr->property()));
    return AssignTarget::make_lvalue(lvalue);
}

TransformResult<AssignTarget> TargetVisitor::target_for(NotNull<AstElementExpr*> expr) {
    TIRO_DEBUG_ASSERT(expr->access_type() == AccessType::Normal,
        "Cannot use optional chaining expressions as the left hand side to an assignment.");

    auto array_result = bb_.compile_expr(TIRO_NN(expr->instance()));
    if (!array_result)
        return array_result.failure();

    auto element_result = bb_.compile_expr(TIRO_NN(expr->element()));
    if (!element_result)
        return element_result.failure();

    auto lvalue = LValue::make_index(*array_result, *element_result);
    return AssignTarget::make_lvalue(lvalue);
}

TransformResult<AssignTarget> compile_target(NotNull<AstExpr*> expr, CurrentBlock& bb) {
    TargetVisitor visitor(bb);
    return visitor.run(expr);
}

TransformResult<std::vector<AssignTarget>>
compile_tuple_targets(NotNull<AstTupleLiteral*> tuple, CurrentBlock& bb) {
    // TODO: Small vec
    std::vector<AssignTarget> targets;
    targets.reserve(tuple->items().size());

    TargetVisitor visitor(bb);
    for (auto item : tuple->items()) {
        auto target = visitor.run(TIRO_NN(item));
        if (!target)
            return target.failure();

        targets.push_back(std::move(*target));
    }
    return TransformResult(std::move(targets));
}

AssignTarget compile_var_binding_target(NotNull<AstVarBinding*> var, CurrentBlock& bb) {
    const auto& symbols = bb.ctx().symbols();
    return symbols.get_decl(symbol_key(var));
}

std::vector<AssignTarget>
compile_tuple_binding_targets(NotNull<AstTupleBinding*> tuple, CurrentBlock& bb) {
    const auto& symbols = bb.ctx().symbols();

    // TODO: Small vec
    const u32 n = tuple->names().size();
    std::vector<AssignTarget> targets;
    targets.reserve(n);

    for (u32 i = 0; i < n; ++i) {
        auto symbol = symbols.get_decl(symbol_key(tuple, i));
        targets.push_back(symbol);
    }

    return targets;
}

OkResult compile_var_decl(NotNull<AstVarDecl*> decl, CurrentBlock& bb) {
    BindingVisitor visitor(bb);
    for (auto binding : decl->bindings()) {
        auto result = visit(TIRO_NN(binding), visitor);
        if (!result)
            return result.failure();
    }
    return ok;
}

LocalResult compile_compound_assign_expr(
    BinaryOpType op, NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb) {
    auto target = compile_target(lhs, bb);
    if (!target)
        return target.failure();

    auto lhs_value = bb.compile_read(*target);
    auto rhs_value = bb.compile_expr(rhs);
    if (!rhs_value)
        return rhs_value;

    auto result = bb.compile_rvalue(RValue::make_binary_op(op, lhs_value, *rhs_value));
    bb.compile_assign(*target, result);
    return result;
}

LocalResult compile_assign_expr(NotNull<AstExpr*> lhs, NotNull<AstExpr*> rhs, CurrentBlock& bb) {
    auto simple_assign = [&]() -> LocalResult {
        auto target = compile_target(lhs, bb);
        if (!target)
            return target.failure();

        auto rhs_result = bb.compile_expr(rhs);
        if (!rhs_result)
            return rhs_result;

        bb.compile_assign(*target, *rhs_result);
        return rhs_result;
    };

    switch (lhs->type()) {
    case AstNodeType::VarExpr:
    case AstNodeType::PropertyExpr:
    case AstNodeType::ElementExpr:
        return simple_assign();

    case AstNodeType::TupleLiteral: {
        auto lit = TIRO_NN(try_cast<AstTupleLiteral>(lhs));

        auto target_result = compile_tuple_targets(lit, bb);
        if (!target_result)
            return target_result.failure();

        auto rhs_result = bb.compile_expr(rhs);
        if (!rhs_result)
            return rhs_result;

        auto& targets = *target_result;
        for (u32 i = 0, n = targets.size(); i != n; ++i) {
            auto element = bb.compile_rvalue(
                RValue::UseLValue{LValue::make_tuple_field(*rhs_result, i)});
            bb.compile_assign(targets[i], element);
        }

        return rhs_result;
    }

    default:
        TIRO_ERROR("Invalid left hand side argument in assignment: {}.", lhs->type());
    }
}

} // namespace tiro
