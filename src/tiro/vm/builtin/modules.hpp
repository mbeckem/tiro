#ifndef TIRO_VM_BUILTIN_MODULES_HPP
#define TIRO_VM_BUILTIN_MODULES_HPP

#include "tiro/vm/objects/modules.hpp"

namespace tiro::vm {

// TODO submodules should be members of their parent module.
Module create_std_module(Context& ctx);
Module create_io_module(Context& ctx);

} // namespace tiro::vm

#endif // TIRO_VM_BUILTIN_MODULES_HPP
