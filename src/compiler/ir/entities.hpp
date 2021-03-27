#ifndef TIRO_COMPILER_IR_ENTITIES_HPP
#define TIRO_COMPILER_IR_ENTITIES_HPP

#include "common/entities/entity_id.hpp"
#include "compiler/ir/fwd.hpp"

namespace tiro::ir {

TIRO_DEFINE_ENTITY_ID(ModuleMemberId, u32)
TIRO_DEFINE_ENTITY_ID(BlockId, u32)
TIRO_DEFINE_ENTITY_ID(ParamId, u32)
TIRO_DEFINE_ENTITY_ID(FunctionId, u32)
TIRO_DEFINE_ENTITY_ID(InstId, u32)
TIRO_DEFINE_ENTITY_ID(LocalListId, u32)
TIRO_DEFINE_ENTITY_ID(RecordId, u32)

} // namespace tiro::ir

#endif // TIRO_COMPILER_IR_ENTITIES_HPP
