#ifndef HAMMER_VM_LOAD_HPP
#define HAMMER_VM_LOAD_HPP

#include "hammer/compiler/output.hpp"
#include "hammer/vm/objects/modules.hpp"
#include "hammer/vm/objects/object.hpp"

namespace hammer::vm {

/// Converts a compiled module to a module object.
Module load_module(
    Context& ctx, const CompiledModule& module, const StringTable& strings);

} // namespace hammer::vm

#endif // HAMMER_VM_LOAD_HPP
