#ifndef TIRO_BYTECODE_FWD_HPP
#define TIRO_BYTECODE_FWD_HPP

#include "tiro/core/defs.hpp"

namespace tiro::bytecode {

enum class Opcode : u8;

class ModuleMemberID;
class FunctionID;

enum class FunctionType : u8;
class Function;

enum class ModuleMemberType : u8;
class ModuleMember;
class Module;

class LocalIndex;
class ParamIndex;
class ModuleIndex;
class Instruction;

} // namespace tiro::bytecode

#endif // TIRO_BYTECODE_FWD_HPP
