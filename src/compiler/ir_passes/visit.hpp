#ifndef TIRO_COMPILER_IR_PASSES_VISIT_HPP
#define TIRO_COMPILER_IR_PASSES_VISIT_HPP

#include "common/adt/function_ref.hpp"
#include "common/defs.hpp"
#include "compiler/ir/fwd.hpp"

namespace tiro::ir {

// Visit all insts used or defined in the given entity.
void visit_insts(const Function& func, const Block& block, FunctionRef<void(InstId)> cb);
void visit_insts(const Function& func, const Terminator& term, FunctionRef<void(InstId)> cb);
void visit_insts(const Function& func, const LValue& lvalue, FunctionRef<void(InstId)> cb);
void visit_insts(const Function& func, const Value& value, FunctionRef<void(InstId)> cb);
void visit_insts(const Function& func, const Inst& local, FunctionRef<void(InstId)> cb);
void visit_insts(const Function& func, const Phi& phi, FunctionRef<void(InstId)> cb);
void visit_insts(const Function& func, const LocalList& list, FunctionRef<void(InstId)> cb);

/// Visits all insts that are used as operands in the given instruction.
void visit_inst_operands(const Function& func, InstId inst, FunctionRef<void(InstId)> cb);

} // namespace tiro::ir

#endif // TIRO_COMPILER_IR_PASSES_VISIT_HPP
