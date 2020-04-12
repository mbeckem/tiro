#ifndef TIRO_IR_ID_HPP
#define TIRO_IR_ID_HPP

#include "tiro/core/id_type.hpp"
#include "tiro/ir/fwd.hpp"

namespace tiro {

TIRO_DEFINE_ID(ModuleMemberID, u32)
TIRO_DEFINE_ID(BlockID, u32)
TIRO_DEFINE_ID(ParamID, u32)
TIRO_DEFINE_ID(FunctionID, u32)
TIRO_DEFINE_ID(LocalID, u32)
TIRO_DEFINE_ID(PhiID, u32)
TIRO_DEFINE_ID(LocalListID, u32)

} // namespace tiro

#endif // TIRO_IR_ID_HPP
