#include "compiler/ir_gen/compile.hpp"

#include "compiler/diagnostics.hpp"
#include "compiler/ir_gen/const_eval.hpp"
#include "compiler/ir_gen/module.hpp"

#include "absl/container/inlined_vector.h"

#include <optional>
#include <utility>

namespace tiro::ir {

namespace {

/// Takes a value and compiles it down to an instruction. Implements some
/// ad-hoc peephole optimizations:
///
/// - Values already computed within a block are reused (local value numbering)
/// - Constants within a block are propagated
/// - Useless copies are avoided
class ValueCompiler final : Transformer {
public:
    ValueCompiler(FunctionIRGen& ctx, BlockId block_id);
    ~ValueCompiler();

    InstId compile(const Value& value);

    SourceRange range() const {
        return {}; // TODO: Needed for diagnostics
    }

public:
    InstId visit_read(const Value::Read& read);
    InstId visit_write(const Value::Write& write);
    InstId visit_alias(const Value::Alias& alias);
    InstId visit_publish_assign(const Value::PublishAssign& pub);
    InstId visit_phi(const Value::Phi& phi);
    InstId visit_observe_assign(const Value::ObserveAssign& obs);
    InstId visit_constant(const Constant& constant);
    InstId visit_outer_environment(const Value::OuterEnvironment& env);
    InstId visit_binary_op(const Value::BinaryOp& binop);
    InstId visit_unary_op(const Value::UnaryOp& unop);
    InstId visit_call(const Value::Call& call);
    InstId visit_aggregate(const Value::Aggregate& agg);
    InstId visit_get_aggregate_member(const Value::GetAggregateMember& get);
    InstId visit_method_call(const Value::MethodCall& call);
    InstId visit_make_environment(const Value::MakeEnvironment& make_env);
    InstId visit_make_closure(const Value::MakeClosure& make_closure);
    InstId visit_make_iterator(const Value::MakeIterator& make_iterator);
    InstId visit_record(const Value::Record& record);
    InstId visit_container(const Value::Container& cont);
    InstId visit_format(const Value::Format& format);
    InstId visit_error(const Value::Error& error);
    InstId visit_nop(const Value::Nop& nop);

private:
    std::optional<Constant> try_eval_binary(BinaryOpType op, InstId lhs, InstId rhs);
    std::optional<Constant> try_eval_unary(UnaryOpType op, InstId value);

    void report(std::string_view which, const EvalResult& result);

    std::optional<ComputedValue> lvalue_cache_key(const LValue& lvalue);
    bool constant_module_member(ModuleMemberId member_id);

    InstId compile_env(ClosureEnvId env);
    InstId define_new(Value&& value);
    InstId memoize_value(const ComputedValue& key, FunctionRef<InstId()> compute);

    const Value& value_of(InstId inst) const;

private:
    BlockId block_id_;
};

} // namespace

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

static Value::BinaryOp commutative_order(const Value::BinaryOp& binop) {
    if (!is_commutative(binop.op))
        return binop;

    auto result = binop;
    if (result.left > result.right)
        std::swap(result.left, result.right);
    return result;
}

ValueCompiler::ValueCompiler(FunctionIRGen& ctx, BlockId block_id)
    : Transformer(ctx)
    , block_id_(block_id) {}

ValueCompiler::~ValueCompiler() {}

InstId ValueCompiler::compile(const Value& value) {
    return value.visit(*this);
}

InstId ValueCompiler::visit_read(const Value::Read& read) {
    // In general, lvalue access causes side effects (e.g. null dereference) and cannot
    // be optimized. In some cases (module level constants, imports) values only have to be computed once
    // and can be cached.
    auto key = lvalue_cache_key(read.target);
    if (!key)
        return define_new(read);

    return memoize_value(*key, [&]() { return define_new(read); });
}

InstId ValueCompiler::visit_write(const Value::Write& write) {
    return define_new(write);
}

InstId ValueCompiler::visit_alias(const Value::Alias& alias) {
    // Collapse useless chains of alias values. We can just use the original instruction.
    // These values can appear, for example, when phi nodes are optimized out.
    InstId target = alias.target;
    while (1) {
        const auto& inst = ctx().result()[target];
        const auto& value = inst.value();
        if (value.type() != ValueType::Alias) {
            break;
        }

        target = value.as_alias().target;
    }
    return target;
}

InstId ValueCompiler::visit_publish_assign(const Value::PublishAssign& pub) {
    return define_new(pub);
}

InstId ValueCompiler::visit_phi(const Value::Phi& phi) {
    // Phi nodes cannot be optimized (in general) because not all predecessors of the block
    // are known. Other parts of the ir transformation phase already take care not to
    // emit useless phi nodes.
    return define_new(phi);
}

InstId ValueCompiler::visit_observe_assign(const Value::ObserveAssign& obs) {
    return define_new(obs);
}

InstId ValueCompiler::visit_constant(const Constant& constant) {
    const auto key = ComputedValue::make_constant(constant);
    return memoize_value(key, [&]() { return define_new(constant); });
}

InstId ValueCompiler::visit_outer_environment([[maybe_unused]] const Value::OuterEnvironment& env) {
    return compile_env(ctx().outer_env());
}

InstId ValueCompiler::visit_binary_op(const Value::BinaryOp& original_binop) {
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

InstId ValueCompiler::visit_unary_op(const Value::UnaryOp& unop) {
    const auto key = ComputedValue::make_unary_op(unop.op, unop.operand);
    return memoize_value(key, [&]() {
        if (const auto constant = try_eval_unary(unop.op, unop.operand))
            return compile(*constant);
        return define_new(unop);
    });
}

InstId ValueCompiler::visit_call(const Value::Call& call) {
    return define_new(call);
}

InstId ValueCompiler::visit_aggregate(const Value::Aggregate& agg) {
    // Improvement: it would be nice if we cache cache the method handles for an instance
    // like we do for unary and binary operations.
    // This is not possible with dynamic typing (in general) because the function property
    // might be reassigned. With static types, this would only happen for function fields.
    return define_new(agg);
}

InstId ValueCompiler::visit_get_aggregate_member(const Value::GetAggregateMember& get) {
    TIRO_DEBUG_ASSERT(
        value_of(get.aggregate).type() == ValueType::Aggregate, "Argument must be an aggregate.");
    TIRO_DEBUG_ASSERT(aggregate_type(get.member) == value_of(get.aggregate).as_aggregate().type(),
        "Type mismatch in aggregate member access.");

    const auto key = ComputedValue::make_aggregate_member_read(get.aggregate, get.member);
    return memoize_value(key, [&]() { return define_new(get); });
}

InstId ValueCompiler::visit_method_call(const Value::MethodCall& call) {
    TIRO_DEBUG_ASSERT(value_of(call.method).as_aggregate().type() == AggregateType::Method,
        "method must be an aggregate.");
    return define_new(call);
}

InstId ValueCompiler::visit_make_environment(const Value::MakeEnvironment& make_env) {
    return define_new(make_env);
}

InstId ValueCompiler::visit_make_closure(const Value::MakeClosure& make_closure) {
    return define_new(make_closure);
}

InstId ValueCompiler::visit_make_iterator(const Value::MakeIterator& make_iterator) {
    return define_new(make_iterator);
}

InstId ValueCompiler::visit_record(const Value::Record& record) {
    return define_new(record);
}

InstId ValueCompiler::visit_container(const Value::Container& cont) {
    return define_new(cont);
}

InstId ValueCompiler::visit_format(const Value::Format& format) {
    // Attempt to find contiguous ranges of constants within the argument list.
    auto& args = ctx().result()[format.args];

    bool args_modified = false;
    LocalList new_args;
    absl::InlinedVector<Constant, 8> constants;

    auto take_constants = [&](size_t i, size_t n) {
        constants.clear();
        while (i != n) {
            const auto& value = value_of(args.get(i));
            if (value.type() != ValueType::Constant)
                break;

            constants.push_back(value.as_constant());
            ++i;
        }
        return i;
    };

    const size_t size = args.size();
    size_t pos = 0;
    while (pos != size) {
        const size_t taken = take_constants(pos, size) - pos;
        if (taken <= 1) {
            new_args.append(args.get(pos));
            ++pos;
            continue;
        }

        auto result = eval_format({constants.data(), constants.size()}, ctx().strings());
        if (!result) {
            report("format", result);
            for (size_t i = 0; i < taken; ++i)
                new_args.append(args.get(pos));
            pos += taken;
            continue;
        }

        new_args.append(compile(*result));
        args_modified = true;
        pos += taken;
    }

    // If only a single string remains, return that string. Otherwise: format.
    if (new_args.size() == 1) {
        auto front_id = new_args[0];
        const auto& front_value = value_of(new_args[0]);
        if (front_value.type() == ValueType::Constant
            && front_value.as_constant().type() == ConstantType::String) {
            return front_id;
        }
    }

    if (args_modified) {
        args = std::move(new_args);
    }
    return define_new(format);
}

InstId ValueCompiler::visit_error(const Value::Error& error) {
    return define_new(error);
}

InstId ValueCompiler::visit_nop(const Value::Nop& nop) {
    return define_new(nop);
}

std::optional<Constant> ValueCompiler::try_eval_binary(BinaryOpType op, InstId lhs, InstId rhs) {
    const auto& left_value = value_of(lhs);
    const auto& right_value = value_of(rhs);
    if (left_value.type() != ValueType::Constant || right_value.type() != ValueType::Constant)
        return {};

    auto result = eval_binary_operation(op, left_value.as_constant(), right_value.as_constant());
    if (!result) {
        report("binary operation", result);
        return {};
    }
    return *result;
}

std::optional<Constant> ValueCompiler::try_eval_unary(UnaryOpType op, InstId operand) {
    const auto& operand_value = value_of(operand);
    if (operand_value.type() != ValueType::Constant)
        return {};

    auto result = eval_unary_operation(op, operand_value.as_constant());
    if (!result) {
        report("unary operation", result);
        return {};
    }
    return *result;
}

void ValueCompiler::report(std::string_view which, const EvalResult& result) {
    switch (result.type()) {
    case EvalResultType::Value:
        TIRO_UNREACHABLE("Result must represent an error.");
        break;

    case EvalResultType::IntegerOverflow:
        diag().reportf(
            Diagnostics::Warning, range(), "Integer overflow in constant evaluation of {}.", which);
        break;

    case EvalResultType::DivideByZero:
        diag().reportf(
            Diagnostics::Warning, range(), "Division by zero in constant evaluation of {}.", which);
        break;

    case EvalResultType::NegativeShift:
        diag().reportf(Diagnostics::Warning, range(),
            "Bitwise shift by a negative amount in constant evaluation of {}.", which);
        break;

    case EvalResultType::ImaginaryPower:
        diag().reportf(
            Diagnostics::Warning, range(), "Imaginary result in constant evaluation of {}.", which);
        break;

    case EvalResultType::TypeError:
        diag().reportf(
            Diagnostics::Warning, range(), "Invalid types in constant evaluation of {}.", which);
        break;
    }
}

std::optional<ComputedValue> ValueCompiler::lvalue_cache_key(const LValue& lvalue) {
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

bool ValueCompiler::constant_module_member(ModuleMemberId member_id) {
    auto symbol_id = ctx().module_gen().find_definition(member_id);
    TIRO_CHECK(symbol_id, "Module member id does not have an associated symbol.");

    const auto& symbol = ctx().symbols()[symbol_id];
    return symbol.is_const();
}

InstId ValueCompiler::compile_env(ClosureEnvId env) {
    return ctx().compile_env(env, block_id_);
}

InstId ValueCompiler::define_new(Value&& value) {
    return ctx().define_new(std::move(value), block_id_);
}

InstId ValueCompiler::memoize_value(const ComputedValue& key, FunctionRef<InstId()> compute) {
    return ctx().memoize_value(key, compute, block_id_);
}

const Value& ValueCompiler::value_of(InstId inst) const {
    return ctx().result()[inst].value();
}

InstId compile_value(const Value& value, CurrentBlock& bb) {
    ValueCompiler gen(bb.ctx(), bb.id());
    auto inst = gen.compile(value);
    TIRO_DEBUG_ASSERT(inst, "Compiled values must produce valid insts.");
    return inst;
}

} // namespace tiro::ir
