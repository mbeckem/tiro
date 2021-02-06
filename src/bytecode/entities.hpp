#ifndef TIRO_BYTECODE_ENTITIES_HPP
#define TIRO_BYTECODE_ENTITIES_HPP

#include "common/id_type.hpp"

namespace tiro {

TIRO_DEFINE_ID(BytecodeFunctionId, u32)
TIRO_DEFINE_ID(BytecodeRecordTemplateId, u32)
TIRO_DEFINE_ID(BytecodeRegister, u32)
TIRO_DEFINE_ID(BytecodeParam, u32)
TIRO_DEFINE_ID(BytecodeMemberId, u32)
TIRO_DEFINE_ID(BytecodeOffset, u32)

} // namespace tiro

#endif // TIRO_BYTECODE_ENTITIES_HPP
