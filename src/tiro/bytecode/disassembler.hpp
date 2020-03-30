#ifndef TIRO_BYTECODE_DISASSEMBLER_HPP
#define TIRO_BYTECODE_DISASSEMBLER_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/span.hpp"

#include <string>

namespace tiro {

/// Disassembles the given bytecode span (which must contain valid bytecode) into a readable string.
std::string disassemble(Span<const byte> bytecode);

} // namespace tiro

#endif // TIRO_BYTECODE_DISASSEMBLER_HPP
