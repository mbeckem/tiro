#ifndef TIRO_VM_HEAP_FWD_HPP
#define TIRO_VM_HEAP_FWD_HPP

namespace tiro::vm {

class Header;
class Heap;
class Collector;
class ObjectList;

namespace new_heap {

class Chunk;
class Page;
class LargeObject;
class HeapAllocator;
class Heap;
class Collector;
struct PageLayout;

} // namespace new_heap

} // namespace tiro::vm

#endif // TIRO_VM_HEAP_FWD_HPP
