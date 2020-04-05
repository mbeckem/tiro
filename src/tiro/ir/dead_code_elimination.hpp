#ifndef TIRO_IRUSED_LOCALS_HPP
#define TIRO_IRUSED_LOCALS_HPP

#include "tiro/ir/fwd.hpp"

namespace tiro {

/// Removes unneeded code from the given function.
/// Local definitions that do not have side effects will be eliminated.
///
/// TODO: Remove dead branches from the CFG.
void eliminate_dead_code(Function& func);

} // namespace tiro

#endif // TIRO_IRUSED_LOCALS_HPP
