#ifndef TIRO_BYTECODE_FWD_HPP
#define TIRO_BYTECODE_FWD_HPP

#include "tiro/core/defs.hpp"

namespace tiro {

class CompiledFunctionID;
enum class CompiledFunctionType : u8;
class CompiledFunction;

enum class CompiledModuleMemberType : u8;
class CompiledModuleMember;
class CompiledModule;

class CompiledOffset;
class CompiledLocalID;
class CompiledParamID;
class CompiledModuleMemberID;

enum class Opcode : u8;
class Instruction;

} // namespace tiro

#endif // TIRO_BYTECODE_FWD_HPP
