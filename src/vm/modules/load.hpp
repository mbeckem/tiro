#ifndef TIRO_VM_MODULES_LOAD_HPP
#define TIRO_VM_MODULES_LOAD_HPP

#include "bytecode/fwd.hpp"
#include "vm/fwd.hpp"
#include "vm/objects/module.hpp"

namespace tiro::vm {

/// Converts a compiled module to a module object.
/// Modules created by this function are not initialized, i.e. their imports
/// have not yet been resolved and their initializer function has not been called.
///
/// NOTE: Throws when the module is invalid (internally, verify_module is called).
Module load_module(Context& ctx, const BytecodeModule& module);

} // namespace tiro::vm

#endif // TIRO_VM_MODULES_LOAD_HPP
