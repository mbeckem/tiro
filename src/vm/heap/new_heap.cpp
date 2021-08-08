#include "vm/heap/new_heap.hpp"

#include <new>

namespace tiro::vm::new_heap {

HeapFacts::HeapFacts(size_t page_size_)
    : page_size(page_size_) {
    if (!is_pow2(page_size))
        TIRO_ERROR("page size must be a power of two: {}", page_size_);

    page_size_log = log2(page_size);
    page_mask = ~(static_cast<uintptr_t>(page_size) - 1);
}

HeapAllocator::~HeapAllocator() {}

Heap::Heap(size_t page_size, HeapAllocator& alloc)
    : alloc_(alloc)
    , facts_(page_size) {}

Heap::~Heap() {
    for (auto p : pages_)
        destroy_page(p);
}

NotNull<Page*> Heap::create_page() {
    void* block = alloc_.allocate_aligned(facts_.page_size, facts_.page_size);
    if (!block)
        TIRO_ERROR("failed to allocate page");

    return TIRO_NN(new (block) Page(*this));
}

void Heap::destroy_page(NotNull<Page*> page) {
    static_assert(std::is_trivially_destructible_v<Page>);
    alloc_.free_aligned(static_cast<void*>(page.get()), facts_.page_size, facts_.page_size);
}

NotNull<Page*> Page::from_address(Heap& heap, const void* address) {
    TIRO_DEBUG_ASSERT(address, "invalid address");

    // This rounds down to a multiple of the page size.
    uintptr_t raw_page = reinterpret_cast<uintptr_t>(address) & heap.facts().page_mask;
    return TIRO_NN(reinterpret_cast<Page*>(raw_page));
}

} // namespace tiro::vm::new_heap
