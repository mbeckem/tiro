#ifndef TIRO_VM_LOAD_HPP
#define TIRO_VM_LOAD_HPP

#include "tiro/compiler/output.hpp"
#include "tiro/objects/modules.hpp"

namespace tiro::vm {

/// Converts a compiled module to a module object.
Module load_module(
    Context& ctx, const OldCompiledModule& module, const StringTable& strings);

} // namespace tiro::vm

#endif // TIRO_VM_LOAD_HPP
