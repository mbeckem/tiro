#ifndef TIRO_BYTECODE_FWD_HPP
#define TIRO_BYTECODE_FWD_HPP

#include "tiro/core/defs.hpp"

namespace tiro {

class BytecodeFunctionID;
enum class BytecodeFunctionType : u8;
class BytecodeFunction;

enum class BytecodeMemberType : u8;
class BytecodeMember;
class BytecodeModule;

class BytecodeOffset;
class BytecodeRegister;
class BytecodeParam;
class BytecodeMemberID;

enum class BytecodeOp : u8;
class BytecodeInstr;

} // namespace tiro

#endif // TIRO_BYTECODE_FWD_HPP
