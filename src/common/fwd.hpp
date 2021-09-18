#ifndef TIRO_COMMON_FWD_HPP
#define TIRO_COMMON_FWD_HPP

#include <cstddef>

namespace tiro {

template<typename Ret, typename... Args>
class FunctionRef;

template<typename Ret, typename... Args>
class FunctionRef<Ret(Args...)>;

template<typename T>
class Span;

template<typename T, typename Vec>
class VecPtr;

class Arena;

enum class ByteOrder : int;

template<typename T>
class Ref;

template<typename T>
class WeakRef;

template<std::size_t TagBits>
class TaggedPtr;

template<typename T>
class NotNull;

class FormatStream;
class StringTable;

template<typename Underlying, typename Derived>
class EntityId;

template<typename Value, typename Id>
class EntityStorage;

template<typename Value, typename Id>
class EntityStorageView;

class Error;
class AssertionFailure;
struct SourceLocation;

// TODO More fwd decls

} // namespace tiro

#endif // TIRO_COMMON_FWD_HPP
