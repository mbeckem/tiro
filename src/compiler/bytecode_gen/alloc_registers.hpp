#ifndef TIRO_COMPILER_BYTECODE_GEN_ALLOC_REGISTERS_HPP
#define TIRO_COMPILER_BYTECODE_GEN_ALLOC_REGISTERS_HPP

#include "compiler/bytecode_gen/locations.hpp"
#include "compiler/ir/fwd.hpp"

namespace tiro {

/// Assigns bytecode registers to ssa instructions in the given function.
/// Used when compiling a function from IR to bytecode.
/// Exposed for testing.
BytecodeLocations allocate_locations(const ir::Function& func);

} // namespace tiro

#endif // TIRO_COMPILER_BYTECODE_GEN_ALLOC_REGISTERS_HPP
