#ifndef TIRO_VM_HANDLES_EXTERNAL_HPP
#define TIRO_VM_HANDLES_EXTERNAL_HPP

#include "common/defs.hpp"
#include "vm/handles/fwd.hpp"
#include "vm/handles/handle.hpp"
#include "vm/handles/traits.hpp"
#include "vm/objects/fwd.hpp"
#include "vm/objects/value.hpp"

#include <absl/container/flat_hash_set.h>

#include <bitset>
#include <climits>

namespace tiro::vm {

namespace detail {

constexpr size_t external_slots_per_page(size_t page_bytes) {
    size_t B = page_bytes;    // Block size
    size_t P = sizeof(void*); // Pointer size
    size_t C = 64;            // Bitset representation in bits (pessimistic)
    size_t M = CHAR_BIT;
    return ((M * B) - C + 1) / (1 + M * P);
};

} // namespace detail

/// Implements a set of handles suitable for use in external code.
/// Handles can be allocated and deallocated manually.
/// TODO: Externals should replace globals.
class ExternalStorage final {
public:
    ExternalStorage();
    ~ExternalStorage();

    ExternalStorage(const ExternalStorage&) = delete;
    ExternalStorage& operator=(const ExternalStorage&) = delete;

    /// Allocates a new handle constructed from the given initial value.
    /// The returned handle must be freed.
    template<typename T = detail::DeduceValueType, typename U>
    inline External<detail::DeducedType<T, U>> allocate(U&& initial);

    /// Allocates a new handle initialized with a default constructed `T`.
    /// The returned handle must be freed.
    template<typename T = Value>
    inline External<T> allocate();

    /// Frees a handle previously allocated through one of the `allocate` functions.
    template<typename T>
    inline void free(External<T> handle);

    /// Returns the number of handles that are currently in use.
    size_t used_slots() const { return total_slots() - free_slots(); }

    /// Returns the number of handles that are in the free list.
    size_t free_slots() const { return free_slots_; }

    /// Returns the total number of slots (free and in use).
    size_t total_slots() const { return total_slots_; }

    template<typename Tracer>
    inline void trace(Tracer&& tracer);

private:
    static constexpr size_t page_size = 1 << 12;
    static constexpr size_t page_slots = detail::external_slots_per_page(page_size);

    union Slot {
        Value value;
        Slot* next_free;
    };

    // TODO: Bitset algorithms, required elsewhere as well. Make 'allocated' a raw array
    // with guaranteed size then.
    struct Page {
        Page() {}

        std::bitset<page_slots> allocated;
        Slot slots[];
    };

    Value* allocate_slot();
    void free_slot(Value* handle);

    void link_free_slot(Slot* slot);
    Slot* pop_free_slot();

    static Page* page_from_slot(Slot* slot);
    static bool slot_allocated(Page* page, Slot* slot);
    static size_t slot_index(Page* page, Slot* slot);

private:
    // Contains all allocated pages.
    absl::flat_hash_set<Page*> pages_;

    // Linked list of free slots. Free slots have the "allocated" bit set to free in their page.
    Slot* first_free_ = nullptr;

    size_t free_slots_ = 0;
    size_t total_slots_ = 0;
};

/// A typed and rooted variable with dynamic lifetime, suitable for use in external code.
///
/// Externals are implicitly convertible to immutable handles. Use `mut()` to
/// explicitly convert an external to a mutable handle.
template<typename T>
class External final : public detail::MutHandleOps<T, External<T>>,
                       public detail::EnableUpcast<T, Handle, External<T>> {
public:
    static External from_raw_slot(Value* slot) { return External(slot, InternalTag()); }

    External() = delete;

    External(const External&) = default;
    External& operator=(const External&) = default;

    /// Converts this external to a mutable handle that points to the same slot.
    MutHandle<T> mut() { return MutHandle<T>::from_raw_slot(slot_); }

    /// Converts this external to an output handle that points to the same slot.
    OutHandle<T> out() { return OutHandle<T>::from_raw_slot(slot_); }

private:
    friend ExternalStorage;

    struct InternalTag {};

    External(Value* rooted_slot, InternalTag)
        : slot_(rooted_slot) {
        TIRO_DEBUG_ASSERT(slot_, "Invalid slot.");
    }

private:
    friend SlotAccess;

    Value* get_slot() const { return slot_; }

private:
    Value* slot_;
};

template<typename T, typename U>
External<detail::DeducedType<T, U>> ExternalStorage::allocate(U&& initial) {
    using deduced_t = detail::DeducedType<T, U>;
    Value* slot = allocate_slot();
    *slot = static_cast<deduced_t>(unwrap_value(initial));
    return {slot, typename External<deduced_t>::InternalTag()};
}

template<typename T>
External<T> ExternalStorage::allocate() {
    return allocate<T>(T());
}

template<typename T>
void ExternalStorage::free(External<T> handle) {
    free_slot(get_valid_slot(handle));
}

template<typename Tracer>
void ExternalStorage::trace(Tracer&& tracer) {
    for (Page* page : pages_) {
        // TODO: std::bitset does not have efficient iteration
        for (size_t i = 0, n = page->allocated.size(); i != n; ++i) {
            if (page->allocated.test(i)) {
                tracer(page->slots[i].value);
            }
        }
    }
}

} // namespace tiro::vm

#endif // TIRO_VM_HANDLES_EXTERNAL_HPP
