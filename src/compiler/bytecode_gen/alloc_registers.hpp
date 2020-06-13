#ifndef TIRO_COMPILER_BYTECODE_GEN_ALLOC_REGISTERS_HPP
#define TIRO_COMPILER_BYTECODE_GEN_ALLOC_REGISTERS_HPP

#include "compiler/bytecode_gen/locations.hpp"
#include "compiler/ir/function.hpp"

namespace tiro {

/// Assigns bytecode registers to ssa locals in the given function.
/// Used when compiling a function from IR to bytecode.
/// Exposed for testing.
BytecodeLocations allocate_locations(const Function& func);

} // namespace tiro

#endif // TIRO_COMPILER_BYTECODE_GEN_ALLOC_REGISTERS_HPP
