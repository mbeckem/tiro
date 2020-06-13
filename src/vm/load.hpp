#ifndef TIRO_VM_LOAD_HPP
#define TIRO_VM_LOAD_HPP

#include "common/string_table.hpp"
#include "compiler/bytecode/fwd.hpp"
#include "vm/objects/modules.hpp"

namespace tiro::vm {

/// Converts a compiled module to a module object.
Module load_module(Context& ctx, const BytecodeModule& module);

} // namespace tiro::vm

#endif // TIRO_VM_LOAD_HPP
