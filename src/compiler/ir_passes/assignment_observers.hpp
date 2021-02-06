#ifndef TIRO_COMPILER_IR_PASSES_ASSIGNMENT_OBSERVERS_HPP
#define TIRO_COMPILER_IR_PASSES_ASSIGNMENT_OBSERVERS_HPP

#include "compiler/ir/fwd.hpp"

namespace tiro::ir {

/// Connections `publish_assign` instructions with `observe_assign` instructions that need their value.
/// This is a necessary pass to implement exception support, which should run immediately after main IR gen is complete.
///
/// Exception handler blocks must receive the current values of variables used within their scope.
/// But because the "main" IR uses SSA instructions and the actual point an exception is raised cannot be known
/// at compile time, we need a phi-like construct to transfer the current value of a variable to the exception handler.
///
/// Example:
///
///     var a = 1;
///     defer std.print(a);
///     f();
///     a = 2;
///     g();
///
/// When `f()` throws an exception, the defer handler must observe `1`, otherwise it must observe `2`.
/// `publish_assign` and `observe_assign` provide the necessary infrastructure for that.
/// Note that explicit control flow edges are not used to implement exceptions, because almost every
/// statement in tiro could potentially throw an exception.
void connect_assignment_observers(Function& func);

} // namespace tiro::ir

#endif // TIRO_COMPILER_IR_PASSES_ASSIGNMENT_OBSERVERS_HPP
