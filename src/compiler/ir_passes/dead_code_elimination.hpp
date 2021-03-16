#ifndef TIRO_COMPILER_IR_PASSES_DEAD_CODE_ELIMINATION_HPP
#define TIRO_COMPILER_IR_PASSES_DEAD_CODE_ELIMINATION_HPP

#include "compiler/ir/fwd.hpp"

namespace tiro::ir {

/// Removes unneeded code from the given function.
/// Definitions that do not have side effects will be eliminated.
///
/// TODO: Remove dead branches from the CFG, currently only definitions are removed.
void eliminate_dead_code(Function& func);

} // namespace tiro::ir

#endif // TIRO_COMPILER_IR_PASSES_DEAD_CODE_ELIMINATION_HPP
