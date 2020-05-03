#ifndef TIRO_IR_ID_HPP
#define TIRO_IR_ID_HPP

#include "tiro/core/id_type.hpp"
#include "tiro/ir/fwd.hpp"

namespace tiro {

TIRO_DEFINE_ID(ModuleMemberId, u32)
TIRO_DEFINE_ID(BlockId, u32)
TIRO_DEFINE_ID(ParamId, u32)
TIRO_DEFINE_ID(FunctionId, u32)
TIRO_DEFINE_ID(LocalId, u32)
TIRO_DEFINE_ID(PhiId, u32)
TIRO_DEFINE_ID(LocalListId, u32)

} // namespace tiro

#endif // TIRO_IR_ID_HPP
