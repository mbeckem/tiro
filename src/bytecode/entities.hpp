#ifndef TIRO_BYTECODE_ENTITIES_HPP
#define TIRO_BYTECODE_ENTITIES_HPP

#include "common/entities/entity_id.hpp"

namespace tiro {

TIRO_DEFINE_ENTITY_ID(BytecodeMemberId, u32)
TIRO_DEFINE_ENTITY_ID(BytecodeFunctionId, u32)
TIRO_DEFINE_ENTITY_ID(BytecodeRecordSchemaId, u32)

// TODO: These should be strong typedefs (there should be no "invalid" value)
TIRO_DEFINE_ENTITY_ID(BytecodeRegister, u32)
TIRO_DEFINE_ENTITY_ID(BytecodeParam, u32)
TIRO_DEFINE_ENTITY_ID(BytecodeOffset, u32)

} // namespace tiro

#endif // TIRO_BYTECODE_ENTITIES_HPP
