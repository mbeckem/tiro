# Memory Management

The tiro vm uses a custom heap layout for the memory used by tiro objects, which is described in this document.

## Requirements and Constraints

-   The heap should allow for reasonable efficient allocation of new objects.
-   Too many calls to the system (or user specified) allocator should be avoided.
    Large chunks of memory should be used and divided into storage for many small objects instead.
-   As tiro requires a garbage collector, the heap must provide facilities to keep track of dead and live objects.
    [optional, but good: the heap is parsable, i.e. starting from the 'root' of the heap, all allocated objects can be iterated]
-   Memory that was used by dead objects must be reused for new allocations (e.g. free lists)
-   The heap layout should be designed with compaction in mind, to combat fragmentation.
-   Multi-threading and multiple heaps and generational gc are out of scope for now.
    There is one shared heap per vm, and a vm is not used by more than a single OS thread at once.

## Terms

-   **Page**: A large, contiguous chunks of memory used for the allocation of most objects.
-   **Cell**: Pages are divided into small, atomic cells. Every allocated object occupies a number of contiguous cells.
-   **Object**: Objects (e.g. numbers, strings or arrays) are allocated on the heap by allocating a fitting number of free cells from a page.
-   **Object Header**: The first few bytes of every object is always occupied by its header, which contains metadata such as its type.

-   **PageSize**: Variable used for the size of pages within a vm (bytes).
-   **CellSize**: Variable used for the size of cells within a page (bytes)

## Heap

The heap manages a set of pages which are allocated on demand.
It is responsible to keep track of:

-   Allocated pages
-   Allocated large object chunks
-   Free space in pages
-   Statistics and heuristics for garbage collection
    and inspection

When the virtual machine attempts to create an object of size `n` (in bytes),
the heap searches its datastructures for a range of cells large enough to hold `n` bytes, which are then marked as allocated.
If no free space is available, garbage collection _may_ be triggered to find some additional free space
(TODO: heuristics) or a new page may be allocated using the configured allocator.
If all attempts to find some free space fail, the heap will signal a fatal "Out of Memory" Error.

When the vm shuts down, the heap will be destroyed and all allocated pages will be freed.

TODO: Compaction

TODO: Returning pages to the OS

## Page Layout

Pages are obtained by allocating large, contiguous chunks of `PageSize` bytes from the allocator.
They are used to obtain memory for most object types (see also the section about large object chunks).

The first few bytes of a page contain the fixed size page header, which is then followed by a small embedded bitmap.
The size of the bitmap depends on the number of cells. (TODO: Section marking bitmap)
All remaining space within the page is split into cells (TODO: Section cell) which will be used for object allocations.
Cell 0 starts directly after the marking bitmap, and the last cell is located at the end of the page (minus some potential padding).

All pages have the same size, which must always be a power of two.
The specific `PageSize` can be set by the user (the default is 1 MiB, with useful values between 64K and a few M).
A page is always aligned to its size (i.e. the start address is a multiple of `PageSize`).

See also the following diagram:

```plain
Page start, multiple of PageSize --->   |-----------------------------------|
                                        | Page header (e.g. parent pointer) |
                                        |-----------------------------------|
                                        | Marking bitmap                    |
                                        |-----------------------------------|
                                        | Cell 0 | Cell 1 | ....            |
                                        | ...                               |
                                        | ...                        Cell N |
  Page ends after PageSize bytes --->   |-----------------------------------|
```

### Object and page address computations

1. One can easily visit all objects within a page (given a page pointer obtained from the heap) by iterating over the
   cells and consulting the page header and the marking bitmap.

2. Given a pointer to a page-allocated object, one can also obtain a pointer to its containing page in O(1) time.
   All such objects are, by definition, allocated from a series of cells in a page.
   Since all pages start at an address multiple of `PageSize` (and are exactly `PageSize` bytes large),
   one can simply round down the address of an objects to the nearest multiple of `PageSize` to obtain the page pointer.
   Because `PageSize` is a power of two, this can be implemented very efficiently by setting the
   least significant bits of the object pointer to 0 (a simple bitwise AND).

Because garbage collection will frequently visit heap allocated objects in order to set (or inspect) their marking bits,
point 2 is especially important.

### Cells

All cells within a page may be used as object storage.
Unoccupied cells can be used as nodes in a free list in order to find free space efficiently.

In order to minimize internal fragmentation, `CellSize` is set the size of a native pointer (e.g. 4 bytes on x86, 8 on x86_64),
which is currently also the uniform size of all tiro values.

Because cell ranges must also support the `(ptr, size)` values of a free list node, the minimum allocation size is set to 2 cells.
This does not waste much space, because 2 cells are also large enough for an object made up of a header (pointer size) and a single member, making
only the representation of objects without any values slightly inefficient.

TODO: `CellSize == 2 * sizeof (void*)` could also be a good candidate with a little more internal fragmentation.

### Marking bitmap

In this first version, the marking bitmap will contain 1 bit for every cell in the page.
The bitmap will be used by the garbage collector to mark cells as dead or alive when tracing the heap.
After garbage collection is done, they will be used to construct free lists: contiguous ranges of dead cells become free list entries.

There is a trade-off when determining the `CellSize`:  
A larger cell size results in more internal fragmentation but fewer bits in the marking bitmap.
Assuming a `CellSize` of 8 bytes, there will be one bit for every 64 bits of actual object storage, i.e. ~1.6% overhead.

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

-   **Init**: Reset the marking bitmap of all pages to zeroes.
-   **Mark**: All live objects are traced, starting from the roots.

    Examples for roots are:

    -   Global data structures and constants
    -   Static data in modules
    -   Values on coroutine stacks (e.g. local variables, parameters)
    -   Native handles that reference a value

    Whenever an object is encountered, the marking bits that belong to its cells are inspected
    (NOTE: the marking bit for a large object is in the chunk header).
    If the bit is already set to 1, nothing needs to be done as the object was already visited.
    If the bit is set to 0, compute the object's size and set the marking bits of all cells that belong to that object to 1,
    then trace all values reachable from the current object.

-   **Sweep**: Visit all pages and build free lists.

    By inspecting the marking bitmaps of the mark phase, we can easily find contiguous ranges of free cells (runs of zeroes in the bitmap).
    Those ranges are put into the page's free list and registered with the heap.
    Large object chunks are simply freed.

## Open questions and issues

-   Memory is not compacted: because objects will not move in memory, we are vulnerable to memory fragmentation.  
    Segregated free lists would help a bit.
-   No "young generation": most objects will die young. High performance garbage collectors use fast bump-pointer allocation for their young (or "eden") generation.
-   Sequential only. Some parallelization would be possible, however.
-   Coroutine stacks are garbage collected. Moving a coroutine stack during collection (-> compaction) would
    be invalid with the current implementation, as it uses direct pointers into the stack.
-   Native interop requires stable pointers in some cases (possibly long lived for async operations).  
    Even when most of the heap is movable, these native objects must either be pinned (like in C# [1]),
    or be allocated off-heap so they are not touched by the GC.

[1]: https://www.mono-project.com/docs/advanced/garbage-collector/sgen/
