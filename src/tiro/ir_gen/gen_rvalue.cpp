#include "tiro/ir_gen/gen_rvalue.hpp"

#include "tiro/compiler/diagnostics.hpp"
#include "tiro/ir_gen/const_eval.hpp"
#include "tiro/ir_gen/gen_module.hpp"

#include <utility>

namespace tiro {

static bool is_commutative(BinaryOpType op) {
    switch (op) {
    case BinaryOpType::Plus:
    case BinaryOpType::Multiply:
    case BinaryOpType::Equals:
    case BinaryOpType::NotEquals:
    case BinaryOpType::BitwiseAnd:
    case BinaryOpType::BitwiseOr:
    case BinaryOpType::BitwiseXor:
        return true;

    default:
        return false;
    }
}

static RValue::BinaryOp commutative_order(const RValue::BinaryOp& binop) {
    if (!is_commutative(binop.op))
        return binop;

    auto result = binop;
    if (result.left > result.right)
        std::swap(result.left, result.right);
    return result;
}

RValueIRGen::RValueIRGen(FunctionIRGen& ctx, BlockId block_id)
    : ctx_(ctx)
    , block_id_(block_id) {}

RValueIRGen::~RValueIRGen() {}

LocalId RValueIRGen::compile(const RValue& value) {
    return value.visit(*this);
}

LocalId RValueIRGen::visit_use_lvalue(const RValue::UseLValue& use) {
    // In general, lvalue access causes side effects (e.g. null dereference) and cannot
    // be optimized. In some cases (module level constants, imports) values only have to be computed once
    // and can be cached.
    auto key = lvalue_cache_key(use.target);
    if (!key)
        return define_new(use);

    return memoize_value(*key, [&]() { return define_new(use); });
}

LocalId RValueIRGen::visit_use_local(const RValue::UseLocal& use) {
    // Collapse useless chains of UseLocal values. We can just use the original local.
    // These values can appear, for example, when phi nodes are optimized out.
    LocalId target = use.target;
    while (1) {
        auto local = ctx().result()[target];
        const auto& value = local->value();
        if (value.type() != RValueType::UseLocal) {
            break;
        }

        target = value.as_use_local().target;
    }
    return target;
}

LocalId RValueIRGen::visit_phi(const RValue::Phi& phi) {
    // Phi nodes cannot be optimized (in general) because not all predecessors of the block
    // are known. Other parts of the ir transformation phase already take care not to
    // emit useless phi nodes.
    return define_new(phi);
}

LocalId RValueIRGen::visit_phi0(const RValue::Phi0& phi) {
    // See visit_phi().
    return define_new(phi);
}

LocalId RValueIRGen::visit_constant(const Constant& constant) {
    const auto key = ComputedValue::make_constant(constant);
    return memoize_value(key, [&]() { return define_new(constant); });
}

LocalId RValueIRGen::visit_outer_environment([[maybe_unused]] const RValue::OuterEnvironment& env) {
    return compile_env(ctx().outer_env());
}

LocalId RValueIRGen::visit_binary_op(const RValue::BinaryOp& original_binop) {
    const auto binop = commutative_order(original_binop);
    const auto key = ComputedValue::make_binary_op(binop.op, binop.left, binop.right);
    return memoize_value(key, [&]() {
        // TODO: Optimize (i + 3) + 4 to i + (3 + 4)
        //
        // Improvement: In order to do optimizations like "x - x == 0"
        // we would need to have type information (x must be an integer or a float,
        // but not e.g. an array).
        if (const auto constant = try_eval_binary(binop.op, binop.left, binop.right)) {
            return compile(*constant);
        }
        return define_new(binop);
    });
}

LocalId RValueIRGen::visit_unary_op(const RValue::UnaryOp& unop) {
    const auto key = ComputedValue::make_unary_op(unop.op, unop.operand);
    return memoize_value(key, [&]() {
        if (const auto constant = try_eval_unary(unop.op, unop.operand))
            return compile(*constant);
        return define_new(unop);
    });
}

LocalId RValueIRGen::visit_call(const RValue::Call& call) {
    return define_new(call);
}

LocalId RValueIRGen::visit_aggregate(const RValue::Aggregate& agg) {
    // Improvement: it would be nice if we cache cache the method handles for an instance
    // like we do for unary and binary operations.
    // This is not possible with dynamic typing (in general) because the function property
    // might be reassigned. With static types, this would only happen for function fields.
    return define_new(agg);
}

LocalId RValueIRGen::visit_get_aggregate_member(const RValue::GetAggregateMember& get) {
    TIRO_DEBUG_ASSERT(
        value_of(get.aggregate).type() == RValueType::Aggregate, "Argument must be an aggregate.");
    TIRO_DEBUG_ASSERT(aggregate_type(get.member) == value_of(get.aggregate).as_aggregate().type(),
        "Type mismatch in aggregate member access.");

    const auto key = ComputedValue::make_aggregate_member_read(get.aggregate, get.member);
    return memoize_value(key, [&]() { return define_new(get); });
}

LocalId RValueIRGen::visit_method_call(const RValue::MethodCall& call) {
    TIRO_DEBUG_ASSERT(value_of(call.method).as_aggregate().type() == AggregateType::Method,
        "method must be an aggregate.");
    return define_new(call);
}

LocalId RValueIRGen::visit_make_environment(const RValue::MakeEnvironment& make_env) {
    return define_new(make_env);
}

LocalId RValueIRGen::visit_make_closure(const RValue::MakeClosure& make_closure) {
    return define_new(make_closure);
}

LocalId RValueIRGen::visit_container(const RValue::Container& cont) {
    return define_new(cont);
}

LocalId RValueIRGen::visit_format(const RValue::Format& format) {
    // Attempt to find contiguous ranges of constants within the argument list.
    const auto args = ctx().result()[format.args];

    // TODO small vec
    bool args_modified = false;
    LocalList new_args;
    std::vector<Constant> constants;

    auto take_constants = [&](size_t i, size_t n) {
        constants.clear();
        while (i != n) {
            auto value = value_of(args->get(i));
            if (value.type() != RValueType::Constant)
                break;

            constants.push_back(value.as_constant());
            ++i;
        }
        return i;
    };

    const size_t size = args->size();
    size_t pos = 0;
    while (pos != size) {
        const size_t taken = take_constants(pos, size) - pos;
        if (taken <= 1) {
            new_args.append(args->get(pos));
            ++pos;
            continue;
        }

        auto result = eval_format(constants, ctx().strings());
        if (!result) {
            report("format", result);
            for (size_t i = 0; i < taken; ++i)
                new_args.append(args->get(pos));
            pos += taken;
            continue;
        }

        new_args.append(compile(*result));
        args_modified = true;
        pos += taken;
    }

    if (new_args.size() == 1)
        return new_args[0];

    if (args_modified) {
        *args = std::move(new_args);
    }
    return define_new(format);
}

std::optional<Constant> RValueIRGen::try_eval_binary(BinaryOpType op, LocalId lhs, LocalId rhs) {
    const auto& left_value = value_of(lhs);
    const auto& right_value = value_of(rhs);
    if (left_value.type() != RValueType::Constant || right_value.type() != RValueType::Constant)
        return {};

    auto result = eval_binary_operation(op, left_value.as_constant(), right_value.as_constant());
    if (!result) {
        report("binary operation", result);
        return {};
    }
    return *result;
}

std::optional<Constant> RValueIRGen::try_eval_unary(UnaryOpType op, LocalId local) {
    const auto& operand_value = value_of(local);
    if (operand_value.type() != RValueType::Constant)
        return {};

    auto result = eval_unary_operation(op, operand_value.as_constant());
    if (!result) {
        report("unary operation", result);
        return {};
    }
    return *result;
}

void RValueIRGen::report(std::string_view which, const EvalResult& result) {
    switch (result.type()) {
    case EvalResultType::Value:
        TIRO_UNREACHABLE("Result must represent an error.");
        break;

    case EvalResultType::IntegerOverflow:
        diag().reportf(Diagnostics::Warning, source(),
            "Integer overflow in constant evaluation of {}.", which);
        break;

    case EvalResultType::DivideByZero:
        diag().reportf(Diagnostics::Warning, source(),
            "Division by zero in constant evaluation of {}.", which);
        break;

    case EvalResultType::NegativeShift:
        diag().reportf(Diagnostics::Warning, source(),
            "Bitwise shift by a negative amount in constant evaluation of {}.", which);
        break;

    case EvalResultType::ImaginaryPower:
        diag().reportf(Diagnostics::Warning, source(),
            "Imaginary result in constant evaluation of {}.", which);
        break;

    case EvalResultType::TypeError:
        diag().reportf(
            Diagnostics::Warning, source(), "Invalid types in constant evaluation of {}.", which);
        break;
    }
}

std::optional<ComputedValue> RValueIRGen::lvalue_cache_key(const LValue& lvalue) {
    switch (lvalue.type()) {
    case LValueType::Module: {
        auto member_id = lvalue.as_module().member;
        if (constant_module_member(member_id))
            return ComputedValue::make_module_member_id(member_id);
        return {};
    }
    default:
        // Cannot cache reads by default.
        // Improvement: constants in closure env.
        // Improvement: members of imported entities should also be const,
        // because only constant members can be exported. This must be documented in the vm design.
        return {};
    }
}

bool RValueIRGen::constant_module_member(ModuleMemberId member_id) {
    auto symbol_id = ctx().module_gen().find_definition(member_id);
    TIRO_CHECK(symbol_id, "Module member id does not have an associated symbol.");

    auto symbol = ctx().symbols()[symbol_id];
    return symbol->is_const();
}

LocalId RValueIRGen::compile_env(ClosureEnvId env) {
    return ctx().compile_env(env, block_id_);
}

LocalId RValueIRGen::define_new(const RValue& value) {
    return ctx().define_new(value, block_id_);
}

LocalId RValueIRGen::memoize_value(const ComputedValue& key, FunctionRef<LocalId()> compute) {
    return ctx().memoize_value(key, compute, block_id_);
}

RValue RValueIRGen::value_of(LocalId local) const {
    return ctx().result()[local]->value();
}

} // namespace tiro
