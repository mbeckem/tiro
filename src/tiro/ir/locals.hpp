#ifndef TIRO_IR_LOCALS_HPP
#define TIRO_IR_LOCALS_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/function_ref.hpp"
#include "tiro/ir/fwd.hpp"

namespace tiro {

// Visit all locals used or defined in the given entity.
void visit_locals(
    const Function& fs, const Block& block, FunctionRef<void(LocalID)> cb);

void visit_locals(const Function& func, const Terminator& term,
    FunctionRef<void(LocalID)> cb);

void visit_locals(
    const Function& func, const LValue& lvalue, FunctionRef<void(LocalID)> cb);

void visit_locals(
    const Function& func, const RValue& rvalue, FunctionRef<void(LocalID)> cb);

void visit_locals(
    const Function& func, const Local& local, FunctionRef<void(LocalID)> cb);

void visit_locals(
    const Function& func, const Phi& phi, FunctionRef<void(LocalID)> cb);

void visit_locals(
    const Function& func, const LocalList& list, FunctionRef<void(LocalID)> cb);

void visit_locals(
    const Function& func, const Stmt& stmt, FunctionRef<void(LocalID)> cb);

/// Visits all locals that are defined by the given statement.
void visit_definitions(
    const Function& func, const Stmt& stmt, FunctionRef<void(LocalID)> cb);

/// Visits all locals that are used as arguments in the given statement.
void visit_uses(
    const Function& func, const Stmt& stmt, FunctionRef<void(LocalID)> cb);

} // namespace tiro

#endif // TIRO_IR_LOCALS_HPP
