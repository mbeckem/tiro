#include "vm/handles/external.hpp"

#include "common/scope_guards.hpp"
#include "vm/heap/memory.hpp"

#include <new>

namespace tiro::vm {

ExternalStorage::ExternalStorage() {
    constexpr size_t page_bytes_used = sizeof(Page) + page_slots * sizeof(Slot);
    static_assert(page_bytes_used <= page_size, "Page size is computation is wrong.");
    static_assert(std::is_trivially_destructible_v<Page>);
}

ExternalStorage::~ExternalStorage() {
    for (Page* page : pages_)
        deallocate_aligned(page, page_size, page_size);
}

Value* ExternalStorage::allocate_slot() {
    if (TIRO_UNLIKELY(!first_free_)) {
        void* aligned_storage = allocate_aligned(page_size, page_size);
        ScopeExit guard = [&] {
            if (aligned_storage)
                deallocate_aligned(aligned_storage, page_size, page_size);
        };

        Page* page = new (aligned_storage) Page();
        pages_.insert(page);
        aligned_storage = nullptr; // disable guard, ownership is transferred

        total_slots_ += page_slots;
        for (size_t i = 0; i < page_slots; ++i)
            link_free_slot(page->slots + i);
    }

    Slot* slot = pop_free_slot();
    TIRO_DEBUG_ASSERT(slot != nullptr, "Must have free slots available.");

    Page* page = page_from_slot(slot);
    TIRO_DEBUG_ASSERT(!slot_allocated(page, slot), "Slot must be marked as free.");
    page->allocated.set(slot_index(page, slot), true);
    return &slot->value;
}

void ExternalStorage::free_slot(Value* handle) {
    if (!handle)
        return;

    Slot* slot = reinterpret_cast<Slot*>(handle);
    Page* page = page_from_slot(slot);
    TIRO_DEBUG_ASSERT(pages_.count(page) == 1, "Page was not allocated through this instance.");

    page->allocated.set(slot_index(page, slot), false);
    link_free_slot(slot);
}

void ExternalStorage::link_free_slot(Slot* slot) {
    TIRO_DEBUG_ASSERT(!slot_allocated(page_from_slot(slot), slot), "Slot must be marked as free.");
    slot->next_free = first_free_;
    first_free_ = slot;
    free_slots_ += 1;
}

ExternalStorage::Slot* ExternalStorage::pop_free_slot() {
    TIRO_DEBUG_ASSERT(first_free_ != nullptr || free_slots_ == 0,
        "Number of free slots must be zero if the free list is empty.");

    Slot* free = first_free_;
    if (!free)
        return nullptr;

    first_free_ = free->next_free;
    free_slots_ -= 1;
    return free;
}

ExternalStorage::Page* ExternalStorage::page_from_slot(Slot* slot) {
    return static_cast<Page*>(aligned_container_from_member(slot, page_mask));
}

bool ExternalStorage::slot_allocated(Page* page, Slot* slot) {
    size_t index = slot_index(page, slot);
    return page->allocated.test(index);
}

size_t ExternalStorage::slot_index(Page* page, Slot* slot) {
    TIRO_DEBUG_ASSERT(page, "Invalid page.");
    TIRO_DEBUG_ASSERT(slot, "Invalid slot.");
    TIRO_DEBUG_ASSERT(page == aligned_container_from_member(slot, page_mask),
        "Slot must be a member of this page.");
    return slot - page->slots;
}

} // namespace tiro::vm
