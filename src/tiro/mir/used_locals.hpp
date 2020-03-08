#ifndef TIRO_MIR_USED_LOCALS_HPP
#define TIRO_MIR_USED_LOCALS_HPP

#include "tiro/core/function_ref.hpp"
#include "tiro/mir/fwd.hpp"

namespace tiro::compiler::mir_transform {

/// Visits all locals referenced by the given objects. The provided callback
/// will be invoked for every encountered local id.
class LocalVisitor final {
public:
    explicit LocalVisitor(
        const mir::Function& func, FunctionRef<void(mir::LocalID)> cb);

    LocalVisitor(const LocalVisitor&) = delete;
    LocalVisitor& operator=(const LocalVisitor&) = delete;

    void accept(const mir::Block& block);
    void accept(const mir::Terminator& term);
    void accept(const mir::LValue& lvalue);
    void accept(const mir::RValue& rvalue);
    void accept(const mir::Local& local);
    void accept(const mir::Phi& phi);
    void accept(const mir::LocalList& list);
    void accept(const mir::Stmt& stmt);

private:
    void invoke(mir::LocalID local);

private:
    const mir::Function& func_;
    FunctionRef<void(mir::LocalID)> cb_;
};

void remove_unused_locals(mir::Function& func);

} // namespace tiro::compiler::mir_transform

#endif // TIRO_MIR_USED_LOCALS_HPP
