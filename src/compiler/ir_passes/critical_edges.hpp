#ifndef TIRO_COMPILER_IR_PASSES_CRITICAL_EDGES_HPP
#define TIRO_COMPILER_IR_PASSES_CRITICAL_EDGES_HPP

#include "compiler/ir/fwd.hpp"

namespace tiro::ir {

/// Splits all critical edges in func's cfg.
///
/// Critical edges are edges from a source block with multiple successors
/// to a target block with multiple predecessors.
///
/// Edges are split by introducing a new intermediate block on offending edges,
/// thereby creating a block with a single predecessor/successor.
///
/// Returns true if the cfg was changed by this function.
bool split_critical_edges(Function& func);

} // namespace tiro::ir

#endif // TIRO_COMPILER_IR_PASSES_CRITICAL_EDGES_HPP
