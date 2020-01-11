#ifndef HAMMER_VM_LOAD_HPP
#define HAMMER_VM_LOAD_HPP

#include "hammer/compiler/output.hpp"
#include "hammer/vm/objects/modules.hpp"

namespace hammer::vm {

/// Converts a compiled module to a module object.
Module load_module(Context& ctx, const compiler::CompiledModule& module,
    const compiler::StringTable& strings);

} // namespace hammer::vm

#endif // HAMMER_VM_LOAD_HPP
