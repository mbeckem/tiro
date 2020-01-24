#ifndef TIRO_HEAP_FWD_HPP
#define TIRO_HEAP_FWD_HPP

namespace tiro::vm {

class Heap;
class Collector;
class ObjectList;

template<typename T>
class Handle;

template<typename T>
class MutableHandle;

class RootBase;
class GlobalBase;

template<typename T>
class Root;

template<typename T>
class Global;

} // namespace tiro::vm

#endif // TIRO_HEAP_FWD_HPP