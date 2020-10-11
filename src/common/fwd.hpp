#ifndef TIRO_COMMON_FWD_HPP
#define TIRO_COMMON_FWD_HPP

namespace tiro {

template<typename Ret, typename... Args>
class FunctionRef;

template<typename Ret, typename... Args>
class FunctionRef<Ret(Args...)>;

template<typename Value, typename Mapper>
class IndexMap;

template<typename T>
class Span;

template<typename T, typename Vec>
class VecPtr;

// TODO More fwd decls

} // namespace tiro

#endif // TIRO_COMMON_FWD_HPP
