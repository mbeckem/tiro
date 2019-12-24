#ifndef HAMMER_VM_BUILTIN_MODULES_HPP
#define HAMMER_VM_BUILTIN_MODULES_HPP

#include "hammer/vm/objects/modules.hpp"

namespace hammer::vm {

// TODO submodules should be members of their parent module.
Module create_std_module(Context& ctx);
Module create_io_module(Context& ctx);

} // namespace hammer::vm

#endif // HAMMER_VM_BUILTIN_MODULES_HPP
