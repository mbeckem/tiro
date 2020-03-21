#ifndef TIRO_BYTECODE_DISASSEMBLER_HPP
#define TIRO_BYTECODE_DISASSEMBLER_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/span.hpp"

#include <string>

namespace tiro {

std::string disassemble(Span<const byte> bytecode);

} // namespace tiro

#endif // TIRO_BYTECODE_DISASSEMBLER_HPP
