#ifndef TIRO_VM_HEAP_FWD_HPP
#define TIRO_VM_HEAP_FWD_HPP

#include "common/defs.hpp"

namespace tiro::vm {

enum class ChunkType : u8;

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
