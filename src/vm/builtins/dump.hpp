#ifndef TIRO_VM_BUILTINS_DUMP_HPP
#define TIRO_VM_BUILTINS_DUMP_HPP

#include "vm/fwd.hpp"
#include "vm/handles/fwd.hpp"
#include "vm/objects/fwd.hpp"

namespace tiro::vm {

/// Dumps `value` into a string.
///
/// The returned string contains a representation of `value` suitable for debugging.
/// For example, strings are mostly outputted as-is (with non-printable character escaped)
/// and builtin containers will list their items, recursively.
///
/// Cycles will be detected during exectution: objects that have already been visited
/// in the current path on the object graph will be omitted, which is signalled using "...".
///
/// NOTE: The format of a dump is not stable.
///
/// NOTE: This function is currently synchronous but might be user-extendable in the future,
/// which means async execution instead.
/// It should not be called from too many places within the vm - especially where sync execution
/// is required - to make future changes easier.
String dump(Context& ctx, Handle<Value> value, bool pretty);

} // namespace tiro::vm

#endif // TIRO_VM_BUILTINS_DUMP_HPP
