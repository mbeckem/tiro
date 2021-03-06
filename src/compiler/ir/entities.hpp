#ifndef TIRO_COMPILER_IR_ENTITIES_HPP
#define TIRO_COMPILER_IR_ENTITIES_HPP

#include "common/id_type.hpp"
#include "compiler/ir/fwd.hpp"

namespace tiro::ir {

TIRO_DEFINE_ID(ModuleMemberId, u32)
TIRO_DEFINE_ID(BlockId, u32)
TIRO_DEFINE_ID(ParamId, u32)
TIRO_DEFINE_ID(FunctionId, u32)
TIRO_DEFINE_ID(InstId, u32)
TIRO_DEFINE_ID(LocalListId, u32)
TIRO_DEFINE_ID(RecordId, u32)

} // namespace tiro::ir

#endif // TIRO_COMPILER_IR_ENTITIES_HPP
