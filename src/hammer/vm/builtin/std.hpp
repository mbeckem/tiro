#ifndef HAMMER_VM_BUILTIN_STD_HPP
#define HAMMER_VM_BUILTIN_STD_HPP

#include "hammer/vm/objects/value.hpp"

namespace hammer::vm {

Module create_std_module(Context& ctx);

} // namespace hammer::vm

#endif // HAMMER_VM_BUILTIN_STD_HPP
