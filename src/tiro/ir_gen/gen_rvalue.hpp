#ifndef TIRO_IR_GEN_GEN_RVALUE_HPP
#define TIRO_IR_GEN_GEN_RVALUE_HPP

#include "tiro/ir/fwd.hpp"
#include "tiro/ir_gen/gen_func.hpp"

#include <optional>

namespace tiro {

/// Takes an rvalue and compiles it down to a local value. Implements some
/// ad-hoc peephole optimizations:
///
/// - Values already computed within a block are reused (local value numbering)
/// - Constants within a block are propagated
/// - Useless copies are avoided
class RValueIRGen final {
public:
    RValueIRGen(FunctionIRGen& ctx, BlockId blockId);
    ~RValueIRGen();

    FunctionIRGen& ctx() const { return ctx_; }
    Diagnostics& diag() const { return ctx_.diag(); }
    StringTable& strings() const { return ctx_.strings(); }

    LocalId compile(const RValue& value);

    SourceReference source() const {
        return {}; // TODO: Needed for diagnostics
    }

public:
    LocalId visit_use_lvalue(const RValue::UseLValue& use);
    LocalId visit_use_local(const RValue::UseLocal& use);
    LocalId visit_phi(const RValue::Phi& phi);
    LocalId visit_phi0(const RValue::Phi0& phi);
    LocalId visit_constant(const Constant& constant);
    LocalId visit_outer_environment(const RValue::OuterEnvironment& env);
    LocalId visit_binary_op(const RValue::BinaryOp& binop);
    LocalId visit_unary_op(const RValue::UnaryOp& unop);
    LocalId visit_call(const RValue::Call& call);
    LocalId visit_method_handle(const RValue::MethodHandle& method);
    LocalId visit_method_call(const RValue::MethodCall& call);
    LocalId visit_make_environment(const RValue::MakeEnvironment& make_env);
    LocalId visit_make_closure(const RValue::MakeClosure& make_closure);
    LocalId visit_container(const RValue::Container& cont);
    LocalId visit_format(const RValue::Format& format);

private:
    std::optional<Constant>
    try_eval_binary(BinaryOpType op, LocalId lhs, LocalId rhs);

    std::optional<Constant> try_eval_unary(UnaryOpType op, LocalId value);

    void report(std::string_view which, const EvalResult& result);

    LocalId compile_env(ClosureEnvId env);
    LocalId define_new(const RValue& value);
    LocalId
    memoize_value(const ComputedValue& key, FunctionRef<LocalId()> compute);

    RValue value_of(LocalId local) const;

private:
    FunctionIRGen& ctx_;
    BlockId blockId_;
};

} // namespace tiro

#endif // TIRO_IR_GEN_GEN_RVALUE_HPP
