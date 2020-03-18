#ifndef TIRO_MIR_COMPILE_RVALUE_HPP
#define TIRO_MIR_COMPILE_RVALUE_HPP

#include "tiro/mir/fwd.hpp"
#include "tiro/mir/transform_func.hpp"

#include <optional>

namespace tiro {

/// Takes an rvalue at compiles it down to a local value. Implements some
/// ad-hoc peephole optimizations:
///
/// - Values already computed within a block are reused (local value numbering)
/// - Constants within a block are propagated
/// - Useless copies are avoided
class RValueCompiler final {
public:
    RValueCompiler(FunctionContext& ctx, mir::BlockID blockID);
    ~RValueCompiler();

    FunctionContext& ctx() const { return ctx_; }
    Diagnostics& diag() const { return ctx_.diag(); }
    StringTable& strings() const { return ctx_.strings(); }

    mir::LocalID compile(const mir::RValue& value);

    SourceReference source() const {
        return {}; // TODO: Needed for diagnostics
    }

public:
    mir::LocalID visit_use_lvalue(const mir::RValue::UseLValue& use);
    mir::LocalID visit_use_local(const mir::RValue::UseLocal& use);
    mir::LocalID visit_phi(const mir::RValue::Phi& phi);
    mir::LocalID visit_phi0(const mir::RValue::Phi0& phi);
    mir::LocalID visit_constant(const mir::Constant& constant);
    mir::LocalID
    visit_outer_environment(const mir::RValue::OuterEnvironment& env);
    mir::LocalID visit_binary_op(const mir::RValue::BinaryOp& binop);
    mir::LocalID visit_unary_op(const mir::RValue::UnaryOp& unop);
    mir::LocalID visit_call(const mir::RValue::Call& call);
    mir::LocalID visit_method_handle(const mir::RValue::MethodHandle& method);
    mir::LocalID visit_method_call(const mir::RValue::MethodCall& call);
    mir::LocalID
    visit_make_environment(const mir::RValue::MakeEnvironment& make_env);
    mir::LocalID
    visit_make_closure(const mir::RValue::MakeClosure& make_closure);
    mir::LocalID visit_container(const mir::RValue::Container& cont);
    mir::LocalID visit_format(const mir::RValue::Format& format);

private:
    std::optional<mir::Constant>
    try_eval_binary(mir::BinaryOpType op, mir::LocalID lhs, mir::LocalID rhs);

    std::optional<mir::Constant>
    try_eval_unary(mir::UnaryOpType op, mir::LocalID value);

    void report(std::string_view which, const EvalResult& result);

    mir::LocalID compile_env(ClosureEnvID env);
    mir::LocalID define_new(const mir::RValue& value);
    mir::LocalID memoize_value(
        const ComputedValue& key, FunctionRef<mir::LocalID()> compute);

    mir::RValue value_of(mir::LocalID local) const;

private:
    FunctionContext& ctx_;
    mir::BlockID blockID_;
};

} // namespace tiro

#endif // TIRO_MIR_COMPILE_RVALUE_HPP
