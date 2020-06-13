#ifndef TIRO_COMPILER_FWD_HPP
#define TIRO_COMPILER_FWD_HPP

#include "common/ref_counted.hpp"

#include <memory>

namespace tiro {

class Compiler;
class Diagnostics;

class CursorPosition;
class SourceMap;

template<typename T>
struct ResetValue;

class SourceReference;

} // namespace tiro

#endif // TIRO_COMPILER_FWD_HPP
