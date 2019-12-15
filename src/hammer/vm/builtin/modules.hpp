#ifndef HAMMER_VM_BUILTIN_MODULES_HPP
#define HAMMER_VM_BUILTIN_MODULES_HPP

#include "hammer/vm/objects/modules.hpp"

namespace hammer::vm {

Module create_std_module(Context& ctx);

} // namespace hammer::vm

#endif // HAMMER_VM_BUILTIN_MODULES_HPP
