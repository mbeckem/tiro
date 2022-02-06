#ifndef TIRO_VM_BUILTINS_MODULES_HPP
#define TIRO_VM_BUILTINS_MODULES_HPP

#include "vm/objects/module.hpp"

namespace tiro::vm {

/// Creates the module object that contains the standard library.
Module create_std_module(Context& ctx);

} // namespace tiro::vm

#endif // TIRO_VM_BUILTINS_MODULES_HPP
