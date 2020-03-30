#ifndef TIRO_BYTECODE_GEN_FWD_HPP
#define TIRO_BYTECODE_GEN_FWD_HPP

#include "tiro/core/defs.hpp"

namespace tiro {

class BytecodeBuilder;

enum class LinkItemType : u8;
class LinkItem;
class LinkFunction;

enum class CompiledLocationType : u8;
class CompiledLocation;
class CompiledLocations;

} // namespace tiro

#endif // TIRO_BYTECODE_GEN_FWD_HPP
