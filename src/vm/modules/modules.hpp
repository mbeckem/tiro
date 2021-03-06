#ifndef TIRO_VM_MODULES_MODULES_HPP
#define TIRO_VM_MODULES_MODULES_HPP

#include "vm/objects/module.hpp"

namespace tiro::vm {

// TODO submodules should be members of their parent module.
Module create_std_module(Context& ctx);

} // namespace tiro::vm

#endif // TIRO_VM_MODULES_MODULES_HPP
