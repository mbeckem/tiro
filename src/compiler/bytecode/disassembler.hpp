#ifndef TIRO_COMPILER_BYTECODE_DISASSEMBLER_HPP
#define TIRO_COMPILER_BYTECODE_DISASSEMBLER_HPP

#include "common/defs.hpp"
#include "common/span.hpp"

#include <string>

namespace tiro {

/// Disassembles the given bytecode span (which must contain valid bytecode) into a readable string.
std::string disassemble(Span<const byte> bytecode);

} // namespace tiro

#endif // TIRO_COMPILER_BYTECODE_DISASSEMBLER_HPP
