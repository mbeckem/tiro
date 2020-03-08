#include "tiro/mir/compile_rvalue.hpp"

#include "tiro/compiler/diagnostics.hpp"
#include "tiro/mir/eval.hpp"

#include <utility>

namespace tiro::compiler::mir_transform {

static bool is_commutative(mir::BinaryOpType op) {
    switch (op) {
    case mir::BinaryOpType::Plus:
    case mir::BinaryOpType::Multiply:
    case mir::BinaryOpType::Equals:
    case mir::BinaryOpType::NotEquals:
    case mir::BinaryOpType::BitwiseAnd:
    case mir::BinaryOpType::BitwiseOr:
    case mir::BinaryOpType::BitwiseXor:
        return true;

    default:
        return false;
    }
}

static mir::RValue::BinaryOp
commutative_order(const mir::RValue::BinaryOp& binop) {
    if (!is_commutative(binop.op))
        return binop;

    auto result = binop;
    if (result.left > result.right)
        std::swap(result.left, result.right);
    return result;
}

RValueCompiler::RValueCompiler(FunctionContext& ctx, mir::BlockID blockID)
    : ctx_(ctx)
    , blockID_(blockID) {}

RValueCompiler::~RValueCompiler() {}

mir::LocalID RValueCompiler::compile(const mir::RValue& value) {
    return value.visit(*this);
}

mir::LocalID
RValueCompiler::visit_use_lvalue(const mir::RValue::UseLValue& use) {
    // In general, lvalue access causes side effects (e.g. null dereference) and cannot
    // be optimized.
    // Improvement: research some cases where the above is possible.
    return define_new(use);
}

mir::LocalID RValueCompiler::visit_use_local(const mir::RValue::UseLocal& use) {
    // Collapse useless chains of UseLocal values. We can just use the original local.
    // These values can appear, for example, when phi nodes are optimized out.
    mir::LocalID target = use.target;
    while (1) {
        auto local = ctx().result()[target];
        const auto& value = local->value();
        if (value.type() != mir::RValueType::UseLocal) {
            break;
        }

        target = value.as_use_local().target;
    }
    return target;
}

mir::LocalID RValueCompiler::visit_phi(const mir::RValue::Phi& phi) {
    // Phi nodes cannot be optimized (in general) because not all predecessors of the block
    // are known. Other parts of the mir transformation phase already take care not to
    // emit useless phi nodes.
    return define_new(phi);
}

mir::LocalID RValueCompiler::visit_phi0(const mir::RValue::Phi0& phi) {
    // See visit_phi().
    return define_new(phi);
}

mir::LocalID RValueCompiler::visit_constant(const mir::Constant& constant) {
    const auto key = ComputedValue::make_constant(constant);
    return memoize_value(key, [&]() { return define_new(constant); });
}

mir::LocalID RValueCompiler::visit_outer_environment(
    [[maybe_unused]] const mir::RValue::OuterEnvironment& env) {
    return compile_env(ctx().outer_env());
}

mir::LocalID
RValueCompiler::visit_binary_op(const mir::RValue::BinaryOp& original_binop) {
    const auto binop = commutative_order(original_binop);
    const auto key = ComputedValue::make_binary_op(
        binop.op, binop.left, binop.right);
    return memoize_value(key, [&]() {
        // TODO: Optimize (i + 3) + 4 to i + (3 + 4)
        //
        // Improvement: In order to do optimizations like "x - x == 0"
        // we would need to have type information (x must be an integer or a float,
        // but not e.g. an array).
        if (const auto constant = try_eval_binary(
                binop.op, binop.left, binop.right)) {
            return compile(*constant);
        }
        return define_new(binop);
    });
}

mir::LocalID RValueCompiler::visit_unary_op(const mir::RValue::UnaryOp& unop) {
    const auto key = ComputedValue::make_unary_op(unop.op, unop.operand);
    return memoize_value(key, [&]() {
        if (const auto constant = try_eval_unary(unop.op, unop.operand))
            return compile(*constant);
        return define_new(unop);
    });
}

mir::LocalID RValueCompiler::visit_call(const mir::RValue::Call& call) {
    return define_new(call);
}

mir::LocalID
RValueCompiler::visit_method_call(const mir::RValue::MethodCall& call) {
    return define_new(call);
}

mir::LocalID RValueCompiler::visit_make_environment(
    const mir::RValue::MakeEnvironment& make_env) {
    return define_new(make_env);
}

mir::LocalID RValueCompiler::visit_make_closure(
    const mir::RValue::MakeClosure& make_closure) {
    return define_new(make_closure);
}

mir::LocalID
RValueCompiler::visit_container(const mir::RValue::Container& cont) {
    return define_new(cont);
}

mir::LocalID RValueCompiler::visit_format(const mir::RValue::Format& format) {
    // Attempt to find contiguous ranges of constants within the argument list.
    const auto args = ctx().result()[format.args];

    // TODO small vec
    bool args_modified = false;
    mir::LocalList new_args;
    std::vector<mir::Constant> constants;

    auto take_constants = [&](size_t i, size_t n) {
        constants.clear();
        while (i != n) {
            auto value = value_of(args->get(i));
            if (value.type() != mir::RValueType::Constant)
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

    if (!args_modified)
        return define_new(format);

    if (new_args.size() == 1)
        return new_args[0];

    const auto new_list_id = ctx().result().make(std::move(new_args));
    return define_new(mir::RValue::make_format(new_list_id));
}

std::optional<mir::Constant> RValueCompiler::try_eval_binary(
    mir::BinaryOpType op, mir::LocalID lhs, mir::LocalID rhs) {
    const auto& left_value = value_of(lhs);
    const auto& right_value = value_of(rhs);
    if (left_value.type() != mir::RValueType::Constant
        || right_value.type() != mir::RValueType::Constant)
        return {};

    auto result = eval_binary_operation(
        op, left_value.as_constant(), right_value.as_constant());
    if (!result) {
        report("binary operation", result);
        return {};
    }
    return *result;
}

std::optional<mir::Constant>
RValueCompiler::try_eval_unary(mir::UnaryOpType op, mir::LocalID local) {
    const auto& operand_value = value_of(local);
    if (operand_value.type() != mir::RValueType::Constant)
        return {};

    auto result = eval_unary_operation(op, operand_value.as_constant());
    if (!result) {
        report("unary operation", result);
        return {};
    }
    return *result;
}

void RValueCompiler::report(std::string_view which, const EvalResult& result) {
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
            "Bitwise shift by a negative amount in constant evaluation of {}.",
            which);
        break;

    case EvalResultType::ImaginaryPower:
        diag().reportf(Diagnostics::Warning, source(),
            "Imaginary result in constant evaluation of {}.", which);
        break;

    case EvalResultType::TypeError:
        diag().reportf(Diagnostics::Warning, source(),
            "Invalid types in constant evaluation of {}.", which);
        break;
    }
}

mir::LocalID RValueCompiler::compile_env(ClosureEnvID env) {
    return ctx().compile_env(env, blockID_);
}

mir::LocalID RValueCompiler::define_new(const mir::RValue& value) {
    return ctx().define_new(value, blockID_);
}

mir::LocalID RValueCompiler::memoize_value(
    const ComputedValue& key, FunctionRef<mir::LocalID()> compute) {
    return ctx().memoize_value(key, compute, blockID_);
}

mir::RValue RValueCompiler::value_of(mir::LocalID local) const {
    return ctx().result()[local]->value();
}

} // namespace tiro::compiler::mir_transform
