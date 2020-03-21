#ifndef TIRO_MIR_GEN_GEN_RVALUE_HPP
#define TIRO_MIR_GEN_GEN_RVALUE_HPP

#include "tiro/mir/fwd.hpp"
#include "tiro/mir_gen/gen_func.hpp"

#include <optional>

namespace tiro {

/// Takes an rvalue and compiles it down to a local value. Implements some
/// ad-hoc peephole optimizations:
///
/// - Values already computed within a block are reused (local value numbering)
/// - Constants within a block are propagated
/// - Useless copies are avoided
class RValueMIRGen final {
public:
    RValueMIRGen(FunctionMIRGen& ctx, BlockID blockID);
    ~RValueMIRGen();

    FunctionMIRGen& ctx() const { return ctx_; }
    Diagnostics& diag() const { return ctx_.diag(); }
    StringTable& strings() const { return ctx_.strings(); }

    LocalID compile(const RValue& value);

    SourceReference source() const {
        return {}; // TODO: Needed for diagnostics
    }

public:
    LocalID visit_use_lvalue(const RValue::UseLValue& use);
    LocalID visit_use_local(const RValue::UseLocal& use);
    LocalID visit_phi(const RValue::Phi& phi);
    LocalID visit_phi0(const RValue::Phi0& phi);
    LocalID visit_constant(const Constant& constant);
    LocalID
    visit_outer_environment(const RValue::OuterEnvironment& env);
    LocalID visit_binary_op(const RValue::BinaryOp& binop);
    LocalID visit_unary_op(const RValue::UnaryOp& unop);
    LocalID visit_call(const RValue::Call& call);
    LocalID visit_method_handle(const RValue::MethodHandle& method);
    LocalID visit_method_call(const RValue::MethodCall& call);
    LocalID
    visit_make_environment(const RValue::MakeEnvironment& make_env);
    LocalID
    visit_make_closure(const RValue::MakeClosure& make_closure);
    LocalID visit_container(const RValue::Container& cont);
    LocalID visit_format(const RValue::Format& format);

private:
    std::optional<Constant>
    try_eval_binary(BinaryOpType op, LocalID lhs, LocalID rhs);

    std::optional<Constant>
    try_eval_unary(UnaryOpType op, LocalID value);

    void report(std::string_view which, const EvalResult& result);

    LocalID compile_env(ClosureEnvID env);
    LocalID define_new(const RValue& value);
    LocalID memoize_value(
        const ComputedValue& key, FunctionRef<LocalID()> compute);

    RValue value_of(LocalID local) const;

private:
    FunctionMIRGen& ctx_;
    BlockID blockID_;
};

} // namespace tiro

#endif // TIRO_MIR_GEN_GEN_RVALUE_HPP
