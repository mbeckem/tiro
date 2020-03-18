#ifndef TIRO_MIR_CONSTRUCT_CSSA_HPP
#define TIRO_MIR_CONSTRUCT_CSSA_HPP

#include "tiro/mir/fwd.hpp"

namespace tiro {

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
bool construct_cssa(mir::Function& func);

} // namespace tiro

#endif // TIRO_MIR_CONSTRUCT_CSSA_HPP
