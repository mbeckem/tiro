#ifndef TIRO_CODEGEN_2_TRANSFORM_HPP
#define TIRO_CODEGEN_2_TRANSFORM_HPP

#include "tiro/bytecode/fwd.hpp"
#include "tiro/mir/fwd.hpp"

namespace tiro::compiler::bytecode {

/// Transforms a module in mir form to a bytecode module.
Module transform(const mir::Module& module);

} // namespace tiro::compiler::bytecode

#endif // TIRO_CODEGEN_2_TRANSFORM_HPP
