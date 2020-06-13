#ifndef TIRO_COMPILER_BYTECODE_GEN_PARALLEL_COPY_HPP
#define TIRO_COMPILER_BYTECODE_GEN_PARALLEL_COPY_HPP

#include "compiler/bytecode/instruction.hpp"
#include "compiler/bytecode_gen/locations.hpp"

namespace tiro {

/// Sequentializes the given set of parallel copies. The result is returned in place.
/// The assignment destinations should be unique.
///
/// After the algorithm has executed, the list of copies can be executed in the order
/// they have been placed in, while preserving parallel copy semantics (i.e. read all inputs
/// before writing outputs). The algorithm may have to allocate spare registers and will
/// call the `alloc_spare` function when needed.
///
/// It is the responsibility of the caller to track and deallocate those spare registers if necessary.
void sequentialize_parallel_copies(
    std::vector<RegisterCopy>& copies, FunctionRef<BytecodeRegister()> alloc_spare);

} // namespace tiro

#endif // TIRO_COMPILER_BYTECODE_GEN_PARALLEL_COPY_HPP
