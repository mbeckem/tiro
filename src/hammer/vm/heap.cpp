#include "hammer/vm/heap.hpp"

#include "hammer/core/defs.hpp"

#include <cstdlib>

namespace hammer::vm {

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
    void* result = std::malloc(size);
    if (!result) {
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
