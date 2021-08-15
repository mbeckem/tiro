# Memory Management

The tiro vm uses a custom heap layout for the memory used by tiro objects, which is described in this document.

## Prior Art

Many techniques and design elements in this document are well known in the garbage collection community or in the literature.
Some elements (e.g. page layout) are based on [LuaJIT's proposed new garbage collector][1] in a heavily simplified manner.

## Requirements and Constraints

-   The heap should allow for reasonable efficient allocation of new objects.
-   Too many calls to the system (or user specified) allocator should be avoided.
    Large chunks of memory should be used and divided into storage for many small objects instead.
-   As tiro requires a garbage collector, the heap must provide facilities to keep track of dead and live objects.
    [optional, but good: the heap is parsable, i.e. starting from the 'root' of the heap, all allocated objects can be iterated]
-   Memory that was used by dead objects must be reused for new allocations (e.g. free lists).
    Heavy fragmentation should be avoided.
-   The heap layout should be designed with compaction in mind, to combat fragmentation in the future.
-   Multi-threading and multiple heaps and generational gc are out of scope for now.
    There is one shared heap per vm, and a vm is not used by more than a single OS thread at once.

## Terms

-   **Heap**: Manages the entire dynamic memory allocated by a single vm instance.
-   **Region**: A collection of pages having the same properties.
-   **Page**: A large, contiguous chunk of memory used for the allocation of most objects.
-   **PageSize**: Variable used for the size of pages within a vm (bytes).
-   **Cell**: Pages are divided into small, atomic cells. Every allocated object occupies a number of contiguous cells.
-   **CellSize**: Variable used for the size of cells within a page (bytes)
-   **Block**: A contiguous series of cells that may serve as object storage or a free list entry.
-   **Object**: Objects (e.g. numbers, strings or arrays) are allocated on the heap by allocating blocks of certain size.
-   **Object Header**: The first few bytes of every object is always occupied by its header, which contains metadata such as its type.

## Heap

The heap manages a set of pages which are allocated on demand.
It is responsible to keep track of:

-   Allocated pages
-   Allocated large object chunks
-   Free space in pages
-   Statistics and heuristics for garbage collection
    and inspection/debugging

When the virtual machine attempts to create an object of size `n` (in bytes),
the heap searches its datastructures for a block large enough to hold `n` bytes, which is then marked as allocated.
If no free space is available, garbage collection _may_ be triggered to find some additional free space,
(TODO: heuristics) or a new page may be allocated using the configured allocator.
If all attempts to find some free space fail, the heap will signal a fatal "Out of Memory" error.

When the vm shuts down, the heap will be destroyed and all allocated pages will be freed.

TODO: Compaction

TODO: Returning pages to the OS

### Regions

Pages allocated by the heap are organized into regions.
All pages within a region share the same properties, which are:

-   Objects require tracing (objects may refer to other objects)
-   Objects require finalization (objects have associated cleanup functions)

The following regions are planned for now:

-   Generic (tracing, no finalizers)
-   Data (no tracing, no finalizers)
-   Finalizers (tracing, finalizers)

    > TODO: Does not need to be traced at the moment, since the only objects with finalizers are native objects,
    > which do not reference other objects.

### Free list

TODO

## Page layout

Pages are obtained by allocating large, contiguous chunks of `PageSize` bytes from the allocator.
They are used to allocate blocks for most object types (see also the section about large object chunks).

The first few bytes of a page contain the fixed size page header, which is then followed by embedded bitmaps.
The size of these bitmaps depends on the number of cells.
All remaining space within the page is split into cells which will be used for object allocations.
Cell 0 starts directly after the bitmaps, and the last cell is located at the end of the page (minus some potential padding).

All pages that belong to a vm have the same size, which must always be a power of two.
The specific `PageSize` can be set by the user (the default is 1 MiB, with useful values between 64K and a few M).
A page is always aligned to its size (i.e. the start address is a multiple of `PageSize`).

See also the following diagram:

```plain
Page start, multiple of PageSize --->   |-----------------------------------|
                                        |           Page header             |
                                        |-----------------------------------|
                                        |  Block bitmap  |  Marking bitmap  |
                                        |-----------------------------------|
                                        | Cell 0 | Cell 1 | ....            |
                                        | ...                               |
                                        | ...                        Cell N |
  Page ends after PageSize bytes --->   |-----------------------------------|
```

### Object and page address computations

1. One can easily visit all objects within a page by iterating over the
   cells and consulting the page header and the block bitmap.

2. Given a pointer to a page-allocated object, one can also obtain a pointer to its containing page in O(1) time.
   All such objects are, by definition, allocated from a series of cells in a page.
   Since all pages start at an address multiple of `PageSize` (and are exactly `PageSize` bytes large),
   one can simply round down the address of an object to the nearest multiple of `PageSize` to obtain the page pointer.

    Because `PageSize` is a power of two, this can be implemented very efficiently by setting the
    least significant bits of the object pointer to 0 (a simple bitwise AND with a precomputed bitmask).

Because garbage collection will frequently visit heap allocated objects in order to set (or inspect) their marking bits,
point 2 is especially important.

### Cells

Cells are small, size aligned blocks of 8 (32 bit systems) or 16 bytes (64 bit systems).

All cells within a page may be used as object storage.
Unoccupied cells may be grouped into blocks in a free list in order to find free space efficiently.

The current value of `CellSize` is twice the size of a native pointer.
This value was chosen so that the alignment of all required data types can be easily supported (doubles can require alignment to 8 byte
boundaries even on 32-bit machines, e.g. on windows).
Cell ranges must also support the `(ptr, size)` values of a free list node, which is also easily done with the current `CellSize`.

The downside of a large cell size is internal fragmentation.
For example, a string can in the worst waste 15 bytes of unused storage on 64 bit platforms.
A more optimized cell layout can be investigated in the future, if necessary.

#### Cell indices

Since cells are size aligned and a power of two, their index within a page can be computed efficiently. Given a cell address `c`, e.g. pointing to the start of an object:

1. Compute the address of the cell's parent page (see Section _Object and page address computations_)
2. Subtract the address of the page's first cell from `c`
3. Shift the result to the right by 3 (32 bit) or 4 (64 bit) bits.

### Bitmaps

There are two embedded bitmaps in every page, with 1 bit each for every cell:

-   the block bitmap keeps track of block boundaries
-   the marking bitmap is used during garbage collection to mark live objects

The following combination of block and mark bit are currently used:

| Block | Mark | Description                                                      |
| ----- | ---- | ---------------------------------------------------------------- |
| 1     | 0    | Dead block (first cell in block)                                 |
| 1     | 1    | Live block (first cell in block)                                 |
| 0     | 0    | Block extent (continuation used by following cells in the block) |
| 0     | 1    | Free block (first cell in block)                                 |

> NOTE: This is exactly the same approach as described in [LuaJIT's wiki][1]

In combination, these two bits allow for efficient marking and parsing of the heap:

-   During the mark phase, only the mark bitmap needs to be updated.
    Only the mark bit of the first cell needs to be touched, as all other cells use `00`
    and will inherit the value.
-   During the sweep phase, dead blocks are discovered by iterating over the bitmap.
    They are grouped into free blocks and put on the free list.
    As they are by definition not reachable by the program, using the mark bit for this does not introduce conflicts.
-   Newly dead blocks can be differentiated from already dead (free) blocks.
    This is helpful when invoking finalizers of native objects.

With 2 bits for every cell, we arrive at an overhead of ~1.5% (64 bit) or ~3% (32 bit).
A larger cell size on 32 bit would reduce overhead, but introduce more internal fragmentation.
The effects of this would best be determined through experiments.

## Large object chunks

Some objects (e.g. strings or arrays) may grow too large to fit into a single page.
Such objects, starting with a certain size (TODO: e.g. `PageSize * 0.25`), will therefore be created in chunks of their own.
Large object chunks start with a small header, followed directly by the object itself.

Large object chunks do not need to be aligned, and their size depends only on the size required by the object.
One can differentiate between 'small' objects allocated on a page and large objects located
in their own chunks by computing the size of that object (TODO: a single header bit might be better?).
Given a large object, one can construct a pointer to its containing chunk by a simple subtraction with the fixed size offset of objects in large object chunks.

Large object chunks are not reused by the heap.
They are allocated on demand and will be freed when the object within them becomes dead.
This is less efficient than the page system, but large objects should be the minority in a production environment.

## Object layout

Objects allocated on the heap live either in a page or in a large object chunk.
The address of any kind of heap allocated object (no matter the size) is always aligned to a multiple of `CellSize`.
As a consequence, the `K` least significant bits (e.g. 2 on x86) in a pointer to an object are always zero
and can be used for additional tag bits.

They always start with an object header, which has the size of one native pointer.
The header is used to record type information and other metadata required by the runtime environment.

The object header looks like this:

```plain
    |------------------------|
    | Type pointer | R1 | R2 |
    |------------------------|
       <-- pointer size -->
```

`R1` and `R2` are one bit each.
Both are reserved for future use, e.g. to signal forward pointers when implementing compaction.
The type pointer points to the internal runtime type of the object, which is also an object (therefore the two bits above are available).

### Determining object size

The size of an object can be determined by starting with the object's type in the object header.
Most types have a fixed size, so usually the object's type will already contain the exact number of bytes occupied by the object.
Some types, like strings or tuples, have variable size which makes inspection of the object's data necessary as well.

A possible optimization of this is described in [LuaJIT's wiki][1]: use an additional bit
to cache the computed size.
The proposed design there also makes marking more efficient, because only a single bit
needs to be changed to mark the cells of a block as live.

### Layout examples

The following figure shows two common object layouts.
All values in this example happen to have the size of a pointer.

```plain

        Instance of Foo                     Tuple
      |------------------|           |-------------------|
      | Header (-> Foo)  |           | Header (-> Tuple) |
      |------------------|           |-------------------|
      | Field1           |           | Size (size_t)     |
      | Field2           |           | Item0             |
      | ...              |           | Item1             |
      | FieldN           |           | ...               |
      |------------------|           | Item (Size - 1)   |
                                     |-------------------|
```

## Garbage collection

The collector is a simple tracing mark and sweep, stop the world, garbage collector.

When garbage collection is triggered, the following phases are executed:

-   **Init**: Reset the mark bit of all objects.
-   **Mark**: All live objects are traced, starting from the roots.

    Examples for roots are:

    -   Global data structures and constants
    -   Static data in modules
    -   Values on coroutine stacks (e.g. local variables, parameters)
    -   Native handles that reference a value

    Whenever an object is encountered, the mark bit that belong to its first cell is inspected
    (NOTE: the mark bit for a large object is in the chunk header).
    If the bit is already set to 1, nothing needs to be done as the object was already visited.
    If the bit is set to 0, set it to 1 and then trace all values reachable from the current object.

    NOTE: Tracing can be skipped if the current page does not contain traceable objects.

-   **Sweep**: Visit all pages and build free lists.

    By inspecting the mark and block bitmaps of the mark phase, we can easily find contiguous ranges of free cells (dead object runs introduced by `10` and followed by `00`s).
    Those ranges are put into the page's free list and registered with the heap.
    Large object chunks are simply freed.

    By using the segregated marking bitset, we make efficient use of the cache during the sweep phase: the object data does not need to be loaded at all.

## Open questions and issues

-   Memory is not compacted: because objects will not move in memory, we are vulnerable to memory fragmentation.  
    Segregated free lists would help a bit.
-   No "young generation": most objects will die young. High performance garbage collectors use fast bump-pointer allocation for their young (or "eden") generation.
-   Sequential only. Some parallelization would be possible, however.
-   Coroutine stacks are garbage collected. Moving a coroutine stack during collection (-> compaction) would
    be invalid with the current implementation, as it uses direct pointers into the stack.
-   Native interop requires stable pointers in some cases (possibly long lived for async operations).  
    Even when most of the heap is movable, these native objects must either be pinned (like in [C#][2]),
    or be allocated off-heap so they are not touched by the GC.

[1]: http://wiki.luajit.org/New-Garbage-Collector
[2]: https://www.mono-project.com/docs/advanced/garbage-collector/sgen/
