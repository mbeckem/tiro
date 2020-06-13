#ifndef TIRO_COMPILER_IR_DEAD_CODE_ELIMINATION_HPP
#define TIRO_COMPILER_IR_DEAD_CODE_ELIMINATION_HPP

#include "compiler/ir/fwd.hpp"

namespace tiro {

/// Removes unneeded code from the given function.
/// Local definitions that do not have side effects will be eliminated.
///
/// TODO: Remove dead branches from the CFG.
void eliminate_dead_code(Function& func);

} // namespace tiro

#endif // TIRO_COMPILER_IR_DEAD_CODE_ELIMINATION_HPP
