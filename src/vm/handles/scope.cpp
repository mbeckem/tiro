#include "vm/handles/scope.hpp"

#include "vm/context.hpp"

#include <new>

namespace tiro::vm {

static constexpr size_t max_pages = 64;

RootedStack::RootedStack() {}

RootedStack::~RootedStack() {
    for (Page* p = first_; p;) {
        Page* next = p->next;
        ::operator delete(static_cast<void*>(p));
        p = next;
    }
}

Value* RootedStack::allocate() {
    // Capacity on current page?
    if (current_ && current_->used < slots_per_page)
        return allocate_from(current_);

    // Leftover storage from a previous expansion?
    if (current_ && current_->next) {
        current_ = current_->next;
        TIRO_DEBUG_ASSERT(current_->used == 0, "Leftover pages must be empty.");
        return allocate_from(current_);
    }

    // TODO: Find a useful default value for "max_pages".
    if (total_pages_ >= max_pages)
        TIRO_ERROR("Managed stack overflowed ({} value slots in use).", used_slots_);

    return allocate_from(new_page());
}

void RootedStack::deallocate(size_t slots) {
    TIRO_DEBUG_ASSERT(slots <= used_slots_, "Cannot deallocate that many elements.");

    size_t remaining = slots;
    while (remaining > 0) {
        TIRO_DEBUG_ASSERT(current_, "Invalid page during deallocation.");
        if (current_->used == 0)
            current_ = current_->prev;

        TIRO_DEBUG_ASSERT(current_, "Invalid page during deallocation.");
        TIRO_DEBUG_ASSERT(current_->used > 0, "Empty page during deallocation.");
        size_t n = std::min(remaining, current_->used);
        current_->used -= n;
        remaining -= n;
    }

    used_slots_ -= slots;
}

RootedStack::Page* RootedStack::new_page() {
    // TODO: Allocator support.
    Page* page = new (::operator new(page_size)) Page();

    if (current_) {
        current_->next = page;
    }
    page->prev = current_;

    current_ = page;
    if (!first_) {
        first_ = page;
    }

    ++total_pages_;
    return page;
}

Value* RootedStack::allocate_from(Page* page) {
    TIRO_DEBUG_ASSERT(page, "Invalid page.");
    TIRO_DEBUG_ASSERT(page->used < slots_per_page, "Page is full.");
    Value* slot = &page->slots[page->used];
    ++page->used;
    ++used_slots_;
    return slot;
}

Scope::Scope(Context& ctx)
    : Scope(ctx.stack()) {}

Scope::Scope(RootedStack& stack)
    : stack_(stack)
    , initial_used_(stack.used_slots())
    , allocated_(0) {}

Scope::~Scope() {
    TIRO_DEBUG_ASSERT(initial_used_ + allocated_ == stack_.used_slots(),
        "Unexpected number of used slots on the stack. The stack must be used in a "
        "stack-like fashion.");
    stack_.deallocate(allocated_);
}

Value* Scope::allocate_slot() {
    TIRO_DEBUG_ASSERT(initial_used_ + allocated_ == stack_.used_slots(),
        "Unexpected number of used slots on the stack. The scope may not be the active "
        "one.");

    Value* slot = stack_.allocate();
    ++allocated_;
    return slot;
}

} // namespace tiro::vm
