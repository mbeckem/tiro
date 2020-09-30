#include "vm/handles/scope.hpp"

#include "vm/context.hpp"

#include <new>

namespace tiro::vm {

static constexpr size_t max_pages = 64;

static_assert(RootedStack::max_slots_per_alloc <= RootedStack::slots_per_page,
    "Singe allocations must fit on a page.");

RootedStack::RootedStack() {}

RootedStack::~RootedStack() {
    for (Page* p = first_; p;) {
        Page* next = p->next;
        ::operator delete(static_cast<void*>(p));
        p = next;
    }
}

Value* RootedStack::allocate() {
    auto span = allocate_slots(1);
    TIRO_DEBUG_ASSERT(span.size() == 1, "Unexpected number of slots allocated.");
    return span.data();
}

void RootedStack::deallocate() {
    return deallocate_slots(1);
}

Span<Value> RootedStack::allocate_slots(size_t slots) {
    if (TIRO_UNLIKELY(slots > max_slots_per_alloc))
        throw std::bad_alloc();

    // Capacity on current page?
    if (current_ && current_->used <= slots_per_page - slots)
        return Span(allocate_from(current_, slots), slots);

    // Leftover storage from a previous expansion?
    if (current_ && current_->next) {
        current_ = current_->next;
        TIRO_DEBUG_ASSERT(current_->used == 0, "Cached pages must be empty.");
        return Span(allocate_from(current_, slots), slots);
    }

    // TODO: Find a useful default value for "max_pages".
    if (total_pages_ >= max_pages)
        TIRO_ERROR("Managed stack overflowed ({} value slots in use).", used_slots_);

    return Span(allocate_from(new_page(), slots), slots);
}

void RootedStack::deallocate_slots(size_t slots) {
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

Value* RootedStack::allocate_from(Page* page, size_t slots) {
    TIRO_DEBUG_ASSERT(page, "Invalid page.");
    TIRO_DEBUG_ASSERT(page->used <= slots_per_page - slots, "Page does not have enough capacity.");
    Value* slot = &page->slots[page->used];
    page->used += slots;
    used_slots_ += slots;
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
    stack_.deallocate_slots(allocated_);
}

Value* Scope::allocate_slot() {
    TIRO_DEBUG_ASSERT(initial_used_ + allocated_ == stack_.used_slots(),
        "Unexpected number of used slots on the stack. The scope may not be the active "
        "one.");

    Value* slot = stack_.allocate();
    ++allocated_;
    return slot;
}

Span<Value> Scope::allocate_slots(size_t n) {
    TIRO_DEBUG_ASSERT(initial_used_ + allocated_ == stack_.used_slots(),
        "Unexpected number of used slots on the stack. The scope may not be the active "
        "one.");

    auto slots = stack_.allocate_slots(n);
    allocated_ += n;
    return slots;
}

} // namespace tiro::vm
