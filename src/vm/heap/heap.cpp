#include "vm/heap/heap.hpp"

#include "common/defs.hpp"
#include "vm/objects/value.hpp"

#include <cstdlib>

namespace tiro::vm {

// Perform collection before every allocation to find memory bugs
// #define TIRO_GC_STRESS

static constexpr bool always_gc_on_allocate =
#ifdef TIRO_GC_STRESS
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

bool Heap::is_pinned([[maybe_unused]] Value v) const {
    return true;
}

void Heap::destroy(Header* hdr) {
    TIRO_DEBUG_ASSERT(hdr, "Invalid object.");

    Value object = Value::from_heap(hdr);
    size_t size = object_size(object);
    finalize(object);

    TIRO_DEBUG_ASSERT(allocated_objects_ >= 1, "Inconsistent counter for allocated objects.");
    allocated_objects_ -= 1;

    free(hdr, size);
}

void* Heap::allocate(size_t size) {
    bool collector_ran = false;

    if (always_gc_on_allocate || allocated_bytes_ >= collector_.next_threshold()) {
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

        // TODO: Custom exception type derived from Error, and ensure
        // that c api forwards allocation error correctly.
        throw std::bad_alloc();
    }

    allocated_bytes_ += size;
    return result;
}

void Heap::free(void* ptr, size_t size) {
    std::free(ptr);
    TIRO_DEBUG_ASSERT(size <= allocated_bytes_, "Inconsistent counter for allocated bytes.");
    allocated_bytes_ -= size;
}

} // namespace tiro::vm
