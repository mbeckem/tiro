#ifndef TIRO_MIR_TRANSFORM_HPP
#define TIRO_MIR_TRANSFORM_HPP

#include "tiro/mir/fwd.hpp"
#include "tiro/syntax/ast.hpp" // TODO ast fwd

namespace tiro::compiler {

mir::Function transform(NotNull<FuncDecl*> func, StringTable& strings);

} // namespace tiro::compiler

#endif // TIRO_MIR_TRANSFORM_HPP
