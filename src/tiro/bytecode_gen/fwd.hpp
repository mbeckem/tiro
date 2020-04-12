#ifndef TIRO_BYTECODE_GEN_FWD_HPP
#define TIRO_BYTECODE_GEN_FWD_HPP

#include "tiro/core/defs.hpp"

namespace tiro {

class BytecodeBuilder;

enum class LinkItemType : u8;
class LinkItem;
class LinkFunction;

enum class BytecodeLocationType : u8;
class BytecodeLocation;
class BytecodeLocations;

} // namespace tiro

#endif // TIRO_BYTECODE_GEN_FWD_HPP
