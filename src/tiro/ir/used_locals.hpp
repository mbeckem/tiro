#ifndef TIRO_IRUSED_LOCALS_HPP
#define TIRO_IRUSED_LOCALS_HPP

#include "tiro/core/function_ref.hpp"
#include "tiro/ir/fwd.hpp"

namespace tiro {

/// Visits all locals referenced by the given objects. The provided callback
/// will be invoked for every encountered local id.
// TODO: Needs unit tests to check that all locals are visited.
class LocalVisitor final {
public:
    explicit LocalVisitor(const Function& func, FunctionRef<void(LocalID)> cb);

    LocalVisitor(const LocalVisitor&) = delete;
    LocalVisitor& operator=(const LocalVisitor&) = delete;

    void accept(const Block& block);
    void accept(const Terminator& term);
    void accept(const LValue& lvalue);
    void accept(const RValue& rvalue);
    void accept(const Local& local);
    void accept(const Phi& phi);
    void accept(const LocalList& list);
    void accept(const Stmt& stmt);

private:
    void invoke(LocalID local);

private:
    const Function& func_;
    FunctionRef<void(LocalID)> cb_;
};

void remove_unused_locals(Function& func);

} // namespace tiro

#endif // TIRO_IRUSED_LOCALS_HPP
