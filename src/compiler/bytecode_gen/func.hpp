#ifndef TIRO_COMPILER_BYTECODE_GEN_FUNC_HPP
#define TIRO_COMPILER_BYTECODE_GEN_FUNC_HPP

#include "bytecode/module.hpp"
#include "compiler/bytecode_gen/object.hpp"
#include "compiler/ir/function.hpp"

namespace tiro {

/// Compiles the given members of the module into a link object.
/// Objects must be linked together to produce the completed bytecode module.
LinkObject compile_object(Module& module, Span<const ModuleMemberId> members);

} // namespace tiro

#endif // TIRO_COMPILER_BYTECODE_GEN_FUNC_HPP
