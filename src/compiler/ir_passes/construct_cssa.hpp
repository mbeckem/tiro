#ifndef TIRO_COMPILER_IR_PASSES_CONSTRUCT_CSSA_HPP
#define TIRO_COMPILER_IR_PASSES_CONSTRUCT_CSSA_HPP

#include "compiler/ir/fwd.hpp"

namespace tiro::ir {

/// Ensures that the function is in CSSA form (no phi function arguments
/// with interfering lifetime).
///
/// Returns true if the cfg was modified.
///
/// References:
///
///     Sreedhar, Vugranam C., Roy Dz-Ching Ju, David M. Gillies and Vatsa Santhanam.
///         Translating Out of Static Single Assignment Form.
///         1999
///
///     Pereira, Fernando Magno Quintão.
///         The Designing and Implementation of A SSA - based register allocator
///         2007
// TODO: This is currently very wasteful with new variables. Should be optimized more
// by implementing the missing parts of the above papers.
bool construct_cssa(Function& func);

} // namespace tiro::ir

#endif // TIRO_COMPILER_IR_PASSES_CONSTRUCT_CSSA_HPP
