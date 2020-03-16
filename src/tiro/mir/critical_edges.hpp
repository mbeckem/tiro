#ifndef TIRO_MIR_CRITICAL_EDGES_HPP
#define TIRO_MIR_CRITICAL_EDGES_HPP

#include "tiro/mir/fwd.hpp"

namespace tiro::compiler {

/// Splits all critical edges in func's cfg.
///
/// Critical edges are edges from a source block with multiple successors
/// to a target block with multiple predecessors.
///
/// Edges are split by introducing a new intermediate block on offending edges,
/// thereby creating a block with a single predecessor/successor.
///
/// Returns true if the cfg was changed by this function.
bool split_critical_edges(mir::Function& func);

} // namespace tiro::compiler

#endif // TIRO_MIR_CRITICAL_EDGES_HPP
