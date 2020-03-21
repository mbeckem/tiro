#ifndef TIRO_BYTECODE_GEN_GEN_MODULE_HPP
#define TIRO_BYTECODE_GEN_GEN_MODULE_HPP

#include "tiro/bytecode/fwd.hpp"
#include "tiro/mir/fwd.hpp"

namespace tiro {

/// Transforms a module in mir form to a bytecode module.
/// Note that the algorithm modifies the input module (CSSA construction,
/// splitting of critical edges, etc.) before generating the final bytecode.
CompiledModule compile_module(Module& module);

} // namespace tiro

#endif // TIRO_BYTECODE_GEN_GEN_MODULE_HPP
