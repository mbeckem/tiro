#ifndef TIRO_CODEGEN_EMITTER_HPP
#define TIRO_CODEGEN_EMITTER_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/not_null.hpp"

#include <vector>

namespace tiro::compiler {

class BasicBlock;

void emit_code(NotNull<BasicBlock*> start, std::vector<byte>& out);

} // namespace tiro::compiler

#endif // TIRO_CODEGEN_EMITTER_HPP
