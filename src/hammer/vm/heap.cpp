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
    finalize(Value::from_heap(hdr));
    free(hdr);
}

void* Heap::allocate(size_t size) {
    return std::malloc(size);
}

void Heap::free(void* ptr) {
    return std::free(ptr);
}

} // namespace hammer::vm
