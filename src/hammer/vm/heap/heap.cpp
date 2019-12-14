#include "hammer/vm/heap/heap.hpp"

#include "hammer/core/defs.hpp"

#include <cstdlib>

namespace hammer::vm {

// Perform collection before every allocation to find memory bugs
// #define HAMMER_GC_STRESS

static constexpr bool always_gc_on_allocate =
#ifdef HAMMER_GC_STRESS
    true;
#else
    false;
#endif

Heap::~Heap() {
    auto cursor = objects_.cursor();
    while (cursor) {
        Header* hdr = cursor.get();
        cursor.remove();
        destroy(hdr);
    }
}

void Heap::destroy(Header* hdr) {
    HAMMER_ASSERT(hdr, "Invalid object.");

    Value object = Value::from_heap(hdr);
    size_t size = object_size(object);
    finalize(object);

    HAMMER_ASSERT(
        allocated_objects_ >= 1, "Inconsistent counter for allocated objects.");
    allocated_objects_ -= 1;

    free(hdr, size);
}

void* Heap::allocate(size_t size) {
    bool collector_ran = false;

    if (always_gc_on_allocate
        || allocated_bytes_ >= collector_.next_threshold()) {
        collector_.collect(*ctx_, GcTrigger::Automatic);
        collector_ran = true;
    }

again:
    void* result = std::malloc(size);
    if (!result) {
        if (!collector_ran) {
            collector_.collect(*ctx_, GcTrigger::AllocFailure);
            collector_ran = true;
            goto again;
        }

        HAMMER_ERROR("Out of memory."); // TODO rework allocation (paged heap)
    }

    allocated_bytes_ += size;
    return result;
}

void Heap::free(void* ptr, size_t size) {
    std::free(ptr);
    HAMMER_ASSERT(
        size <= allocated_bytes_, "Inconsistent counter for allocated bytes.");
    allocated_bytes_ -= size;
}

} // namespace hammer::vm
