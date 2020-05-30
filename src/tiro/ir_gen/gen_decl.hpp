#ifndef TIRO_IR_GEN_GEN_DECL_HPP
#define TIRO_IR_GEN_GEN_DECL_HPP

#include "tiro/ir_gen/gen_func.hpp"

namespace tiro {

OkResult gen_var_decl(NotNull<AstVarDecl*> decl, CurrentBlock& bb);

} // namespace tiro

#endif // TIRO_IR_GEN_GEN_DECL_HPP
