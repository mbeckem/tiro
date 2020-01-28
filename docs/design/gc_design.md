# Garbage collector design notes

## Status

The current garbage collector is extremely simplistic because other parts of the language were more important.  
Every object is allocated through std::malloc on its own. All objects have a header that indicates their types.  
Additionally, all objects are linked together through a pointer in the object header, this makes visiting every object possible.  
A mark bit in the object header allows us to track live objects and free those that have not been marked.

A mark-sweep algorithm visits all live objects and marks those objects (through their mark bit) that are still live.  
The sweep part of the algorithms visits all allocated objects (through the next pointers) and frees those objects that have
not been marked as live.  
This algorithm will be replaced by a better implementation in a future version.

```c++
class Header {
    enum Flags : u32 {
        FLAG_MARKED = 1 << 0,
    };

    u32 class_ = 0;
    u32 flags_ = 0;
    Header* next = nullptr;
};
```

## Mark I (soon TM)

The next version of the garbage collector will have the following properties:

- Objects will not be allocated individually. Instead, large pages of memory (e.g. 64 KiB to 1 MiB) will be allocated at once.  
  Free memory within those pages will be managed through free lists.
- Objects will no longer be linked together in a linked list. Instead, the live objects within a page can be iterated over.
- Mark bits will be stored together with a page. There will be one mark bit for every 8 (or 16?) bytes of memory,  
  arriving at a storage overhead of 1/64 or 1/128. Live objects can be identified quickly by inspecting this bitmap.

The object header will become more compact:
```c++
class Header {
    // Exposition only, actual usage of these fields may vary.
    u32 class_ = 0;
    u32 flags_ = 0;
};
```

The collection algorithm will work as follows:

- Mark: Visit all live objects through the roots in the program, set mark bits of live storage cells to `1`.
- Sweep: Rebuild every page's free list by identifying contiguous runs of `0`s in the marking bitmap.
- Build a small index structure of pages with free storage.

This algorithms is much better than the initial one, but still suffers from some problems:

- Memory is not compacted: because objects will not move in memory, we are vulnerable to memory fragmentation.  
  Segregated free lists would help a bit.
- No "young generation": most objects will die young. High performance garbage collectors use fast bump-pointer  
  allocation for their young (or "eden") generation.
- Sequential only. Some parallelization would be possible, however.

## Mark II (far future)

- Generational
- Compacting for old generation

### Open questions for Mark II:
- Coroutine stacks are garbage collected. Moving a coroutine stack during collection (-> compaction) would  
  be invalid with the current implementation, as it uses direct pointers into the stack.
- Native interop requries stable pointers in some cases (possibly long lived for async operations).  
  Even when most of the heap is movable, these native objects must either be pinned (like in C# [1]), 
  or be allocated off-heap so they are not touched by the GC.

[1]: https://www.mono-project.com/docs/advanced/garbage-collector/sgen/
