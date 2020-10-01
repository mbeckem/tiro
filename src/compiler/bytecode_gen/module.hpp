#ifndef TIRO_COMPILER_BYTECODE_GEN_MODULE_HPP
#define TIRO_COMPILER_BYTECODE_GEN_MODULE_HPP

#include "bytecode/fwd.hpp"
#include "compiler/ir/fwd.hpp"

namespace tiro {

/// Transforms a module in ir form to a bytecode module.
/// Note that the algorithm modifies the input module (CSSA construction,
/// splitting of critical edges, etc.) before generating the final bytecode.
BytecodeModule compile_module(Module& module);

} // namespace tiro

#endif // TIRO_COMPILER_BYTECODE_GEN_MODULE_HPP
