#ifndef TIRO_BYTECODE_GEN_FUNC_HPP
#define TIRO_BYTECODE_GEN_FUNC_HPP

#include "tiro/bytecode/module.hpp"
#include "tiro/bytecode_gen/object.hpp"
#include "tiro/ir/function.hpp"

#include <unordered_map>
#include <vector>

namespace tiro {

/// Compiles the given members of the module into a link object.
/// Objects must be linked together to produce the completed bytecode module.
LinkObject compile_object(Module& module, Span<const ModuleMemberId> members);

} // namespace tiro

#endif // TIRO_BYTECODE_GEN_FUNC_HPP
