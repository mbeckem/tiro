#ifndef TIRO_COMPILER_FWD_HPP
#define TIRO_COMPILER_FWD_HPP

namespace tiro {

class Compiler;
class Diagnostics;

class CursorPosition;
class SourceMap;
class SourceRange;

class SourceId;
class AbsoluteSourceRange;
class SourceDb;

template<typename T>
struct ResetValue;

} // namespace tiro

#endif // TIRO_COMPILER_FWD_HPP
