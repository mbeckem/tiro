#ifndef TIRO_VM_FWD_HPP
#define TIRO_VM_FWD_HPP

#include "vm/handles/fwd.hpp"
#include "vm/heap/fwd.hpp"
#include "vm/modules/fwd.hpp"
#include "vm/objects/fwd.hpp"

namespace tiro::vm {

class Context;
class RootSet;
class TypeSystem;
class Interpreter;
class BytecodeInterpreter;
class Registers;

} // namespace tiro::vm

#endif // TIRO_VM_FWD_HPP
