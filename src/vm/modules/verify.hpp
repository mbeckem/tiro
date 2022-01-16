#ifndef TIRO_VM_MODULES_VERIFY_HPP
#define TIRO_VM_MODULES_VERIFY_HPP

#include "bytecode/fwd.hpp"
#include "vm/fwd.hpp"
#include "vm/objects/module.hpp"

namespace tiro::vm {

/// Maximum number of locals supported in a function.
inline constexpr u32 max_locals = 1 << 14;

/// Verifies the module's content with static checks.
/// Throws an exception if verification fails.
///
/// This function catches many errors caused by invalid code generation
/// ahead of time, eliminating the equivalent runtime checks during bytecode
/// interpretation.
///
/// TODO: no control flow analysis is being done yet (e.g. to verify number of arguments on the stack)
void verify_module(const BytecodeModule& module);

} // namespace tiro::vm

#endif // TIRO_VM_MODULES_VERIFY_HPP
