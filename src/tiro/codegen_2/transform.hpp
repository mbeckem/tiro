#ifndef TIRO_CODEGEN_2_TRANSFORM_HPP
#define TIRO_CODEGEN_2_TRANSFORM_HPP

#include "tiro/bytecode/fwd.hpp"
#include "tiro/mir/fwd.hpp"

namespace tiro::bytecode {

/// Transforms a module in mir form to a bytecode module.
Module transform(const mir::Module& module);

} // namespace tiro::bytecode

#endif // TIRO_CODEGEN_2_TRANSFORM_HPP
