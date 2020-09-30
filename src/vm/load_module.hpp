#ifndef TIRO_VM_LOAD_MODULE_HPP
#define TIRO_VM_LOAD_MODULE_HPP

#include "common/string_table.hpp"
#include "compiler/bytecode/fwd.hpp"
#include "vm/objects/module.hpp"

namespace tiro::vm {

/// Converts a compiled module to a module object.
/// Modules created by this function are not initialized, i.e. their imports
/// have not yet been resolved and their initializer function has not been called.
Module load_module(Context& ctx, const BytecodeModule& module);

} // namespace tiro::vm

#endif // TIRO_VM_LOAD_MODULE_HPP
