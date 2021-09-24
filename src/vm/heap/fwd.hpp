#ifndef TIRO_VM_HEAP_FWD_HPP
#define TIRO_VM_HEAP_FWD_HPP

namespace tiro::vm {

struct HeapStats;
struct PageLayout;

class Header;
class Chunk;
class Page;
class LargeObject;
class FreeSpace;
class HeapAllocator;
class Heap;
class Collector;

} // namespace tiro::vm

#endif // TIRO_VM_HEAP_FWD_HPP
