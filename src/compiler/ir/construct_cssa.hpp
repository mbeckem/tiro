#ifndef TIRO_COMPILER_IR_CONSTRUCT_CSSA_HPP
#define TIRO_COMPILER_IR_CONSTRUCT_CSSA_HPP

#include "compiler/ir/fwd.hpp"

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
//
// TODO: Implement
//      Benoit Boissinot, Alain Darte, Fabrice Rastello, Benoît Dupont de Dinechin, Christophe Guillon.
//          Revisiting Out-of-SSA Translation for Correctness, Code Quality, and Efficiency.
//          [Research Report] 2008, pp.14. ￿inria-00349925v1
bool construct_cssa(Function& func);

} // namespace tiro

#endif // TIRO_COMPILER_IR_CONSTRUCT_CSSA_HPP
