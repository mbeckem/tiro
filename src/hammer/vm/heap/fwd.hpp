#ifndef HAMMER_VM_HEAP_FWD_HPP
#define HAMMER_VM_HEAP_FWD_HPP

namespace hammer::vm {

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

} // namespace hammer::vm

#endif // HAMMER_VM_HEAP_FWD_HPP
