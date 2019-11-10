#ifndef HAMMER_VM_FWD_HPP
#define HAMMER_VM_FWD_HPP

namespace hammer::vm {

class Context;
class Heap;
class Collector;
class ObjectList;

template<typename T>
class Handle;

template<typename T>
class MutableHandle;

class RootBase;

template<typename T>
class Root;

} // namespace hammer::vm

#endif // HAMMER_VM_FWD_HPP
