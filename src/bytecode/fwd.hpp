#ifndef TIRO_BYTECODE_FWD_HPP
#define TIRO_BYTECODE_FWD_HPP

#include "common/defs.hpp"

namespace tiro {

class BytecodeFunctionId;
enum class BytecodeFunctionType : u8;
class BytecodeFunction;

class BytecodeRecordTemplateId;
class BytecodeRecordTemplate;

enum class BytecodeMemberType : u8;
class BytecodeMember;
class BytecodeModule;

class BytecodeOffset;
class BytecodeRegister;
class BytecodeParam;
class BytecodeMemberId;

enum class BytecodeOp : u8;
class BytecodeInstr;

} // namespace tiro

#endif // TIRO_BYTECODE_FWD_HPP
